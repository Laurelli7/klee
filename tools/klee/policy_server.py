#!/usr/bin/env python3
"""
KLEE Policy Server - LLM-powered decision server for LLMGuidedSearcher

This server listens on a Unix socket and responds to strategy queries from KLEE.
It uses LLMs (OpenAI or Anthropic) to decide which search strategy KLEE should
use for each function.

Usage:
    OPENAI_API_KEY=sk-... python policy_server.py --provider openai --verbose
    ANTHROPIC_API_KEY=... python policy_server.py --provider anthropic --verbose

Protocol:
    KLEE sends: {"_schema":{...},"data":{"function":"name",...}}\n
    Server responds: "nurs:covnew\n" (or dfs, bfs, random-path, nurs:md2u, etc.)
"""

import argparse
import json
import os
import socket
import sys
from datetime import datetime

# LLM providers
try:
    import openai
    HAS_OPENAI = True
except ImportError:
    HAS_OPENAI = False
    print("Warning: openai package not installed. Run: pip install openai")

try:
    import anthropic
    HAS_ANTHROPIC = True
except ImportError:
    HAS_ANTHROPIC = False
    print("Warning: anthropic package not installed. Run: pip install anthropic")


# All KLEE searchers available to LLMGuidedSearcher
SEARCHERS = [
    "dfs",           # Depth-first search
    "bfs",           # Breadth-first search
    "random-state",  # Random state selection
    "random-path",   # Random path selection (requires execution tree)
    "nurs:covnew",   # Prioritize new coverage
    "nurs:md2u",     # Minimum distance to uncovered
    "nurs:depth",    # Prioritize by depth
    "nurs:rp",       # Random priority
    "nurs:icnt",     # Instruction count
    "nurs:cpicnt",   # Call path instruction count
    "nurs:qc",       # Query cost
    "empc",          # Exhaustive MPC - inter-procedural path coverage
    "sgs",           # Subpath-Guided Search - interleaved subpath searchers
    "default",       # KLEE default: interleaved random-path + nurs:covnew
]

# Default searcher to use for libc functions (no LLM query)
DEFAULT_LIBC_SEARCHER = "nurs:covnew"

# Libc/system function prefixes - skip LLM for these
LIBC_PREFIXES = (
    "_",        # _start, _init, _stdio_init, etc.
    "__",       # __libc_start_main, __get_file, etc.
)

# Explicit libc function names to skip
LIBC_FUNCTIONS = {
    # String functions
    "strlen", "strcpy", "strncpy", "strcat", "strncat", "strcmp", "strncmp",
    "strchr", "strrchr", "strstr", "strdup", "strndup", "strtok", "strtol",
    "strtoul", "strtod", "strspn", "strcspn", "strpbrk",
    # Memory functions  
    "memcpy", "memmove", "memset", "memcmp", "memchr", "mempcpy",
    "malloc", "calloc", "realloc", "free", "alloca",
    # I/O functions
    "read", "write", "open", "close", "fopen", "fclose", "fread", "fwrite",
    "fgets", "fputs", "fgetc", "fputc", "getc", "putc", "getchar", "putchar",
    "printf", "fprintf", "sprintf", "snprintf", "vprintf", "vfprintf", "vsprintf",
    "scanf", "fscanf", "sscanf", "fflush", "fseek", "ftell", "rewind",
    "feof", "ferror", "clearerr", "fileno", "fdopen", "freopen",
    "puts", "gets", "perror",
    # Terminal/TTY
    "isatty", "tcgetattr", "tcsetattr", "ttyname", "ioctl",
    # Character functions
    "isalpha", "isdigit", "isalnum", "isspace", "isupper", "islower",
    "isprint", "iscntrl", "ispunct", "isxdigit", "isgraph",
    "tolower", "toupper",
    # Process/system
    "exit", "abort", "_exit", "atexit", "getenv", "setenv", "unsetenv",
    "getpid", "getppid", "fork", "execve", "wait", "waitpid",
    "signal", "sigaction", "kill",
    # Other common libc
    "qsort", "bsearch", "abs", "labs", "div", "ldiv",
    "atoi", "atol", "atof", "rand", "srand", "time", "clock",
    "stat", "fstat", "lstat", "access", "chmod", "chown",
    "mkdir", "rmdir", "chdir", "getcwd", "opendir", "readdir", "closedir",
}


def is_libc_function(func_name: str) -> bool:
    """Check if a function is a libc/system function (not application code)."""
    if not func_name:
        return True
    
    # Check prefixes
    for prefix in LIBC_PREFIXES:
        if func_name.startswith(prefix):
            return True
    
    # Check explicit list
    if func_name in LIBC_FUNCTIONS:
        return True
    
    return False


# Global log file handle
_log_file = None


def init_log_file(log_path: str):
    """Initialize log file for writing."""
    global _log_file
    _log_file = open(log_path, 'w')
    log(f"Logging to: {log_path}")


def log(msg: str):
    """Print a timestamped log message to stdout and optionally to file."""
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    line = f"[{timestamp}] {msg}"
    print(line)
    if _log_file:
        _log_file.write(line + "\n")
        _log_file.flush()  # Ensure immediate write

# Default system prompt - gives the LLM persistent context
DEFAULT_SYSTEM_PROMPT = """You are an expert symbolic execution strategist for KLEE.
Your goal is to maximize CODE COVERAGE and TEST CASE GENERATION.
{program_context}

Available searchers:
- dfs: Depth-first search. Good for completing paths and generating test cases.
- bfs: Breadth-first search. Good for broad exploration.
- random-state: Randomly select among active states.
- random-path: Randomly walk the execution tree to select a state.
- nurs:covnew: Prioritize states that cover new code. KLEE's best general-purpose strategy.
- nurs:md2u: Prioritize states closest to uncovered code.
- nurs:depth: Prioritize by execution depth.
- nurs:rp: Random priority weighting.
- nurs:icnt: Prioritize by instruction count.
- nurs:cpicnt: Call path instruction count.
- nurs:qc: Prioritize states with low query cost (fast solver queries).
- empc: Exhaustive inter-procedural path coverage. Best for functions with complex call chains.
- sgs: Subpath-guided search. Good for structured programs with many subpaths.
- default: KLEE's default interleaved random-path + nurs:covnew. A strong baseline.

Key principles:
1. Coverage requires COMPLETING paths (generating ktests), not just exploring
2. Many active states with few completed tests = need DFS to finish paths
3. Coverage stall = try random-path to escape local optima
4. High solver time = use nurs:qc to avoid expensive queries
5. State near termination (low dist_to_return, shallow stack) = prioritize completion with DFS
6. Complex control flow with many branches = try empc or sgs for systematic coverage
7. When unsure, "default" or "nurs:covnew" are safe choices

Always respond with:
REASONING: <brief explanation>
CHOICE: <searcher name>"""


class PolicyServer:
    def __init__(self, socket_path: str, provider: str, model: str = None, 
                 verbose: bool = False, system_prompt: str = None, program: str = None):
        self.socket_path = socket_path
        self.provider = provider
        self.model = model
        self.verbose = verbose
        self.program = program
        self.sock = None
        self.llm_client = None
        self.query_count = 0
        
        # Build system prompt with program context
        if system_prompt:
            self.system_prompt = system_prompt
        else:
            program_context = ""
            if program:
                program_context = f"\nYou are analyzing: **{program}**\n"
            self.system_prompt = DEFAULT_SYSTEM_PROMPT.format(program_context=program_context)
        
        # Initialize LLM client
        if provider == "openai":
            if not HAS_OPENAI:
                raise RuntimeError("openai package not installed")
            self.llm_client = openai.OpenAI()
            self.model = model or "gpt-4o-mini"
        elif provider == "anthropic":
            if not HAS_ANTHROPIC:
                raise RuntimeError("anthropic package not installed")
            self.llm_client = anthropic.Anthropic()
            self.model = model or "claude-3-haiku-20240307"
        else:
            raise ValueError(f"Unknown provider: {provider}. Use 'openai' or 'anthropic'")
    
    def start(self):
        """Start listening on Unix socket."""
        # Remove existing socket file
        if os.path.exists(self.socket_path):
            os.unlink(self.socket_path)
        
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.bind(self.socket_path)
        self.sock.listen(5)
        
        log("=" * 70)
        log("KLEE POLICY SERVER STARTED")
        log("=" * 70)
        log(f"Socket path: {self.socket_path}")
        log(f"LLM Provider: {self.provider}")
        log(f"LLM Model: {self.model}")
        if self.program:
            log(f"Program: {self.program}")
        log(f"Verbose mode: {self.verbose}")
        log("=" * 70)
        log("Waiting for KLEE to connect...")
        
        try:
            while True:
                conn, _ = self.sock.accept()
                log("✓ KLEE CONNECTED")
                self.handle_connection(conn)
        except KeyboardInterrupt:
            log("\nShutting down...")
        finally:
            self.sock.close()
            if os.path.exists(self.socket_path):
                os.unlink(self.socket_path)
    
    def handle_connection(self, conn: socket.socket):
        """Handle a single connection from KLEE."""
        buffer = ""  # Buffer for incomplete messages
        try:
            while True:
                data = conn.recv(16384)  # Large buffer for schema + data
                if not data:
                    break
                
                # Add received data to buffer
                buffer += data.decode()
                
                # Process all complete JSON messages (newline-delimited)
                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    line = line.strip()
                    if not line:
                        continue
                    
                    self.query_count += 1
                    
                    try:
                        request = json.loads(line)
                        
                        # Extract schema and data
                        if "data" in request:
                            features = request["data"]
                            schema = request.get("_schema", {})
                        else:
                            features = request
                            schema = {}
                        
                        func_name = features.get('function', 'unknown')
                        # query_type and health_status are at the top level of the JSON
                        query_type = request.get('query_type', features.get('query_type', 'function'))
                        health_status = request.get('health_status', features.get('health_status', ''))
                        
                        # ============ FILTER: Skip libc functions (only for function queries) ============
                        if query_type == 'function' and is_libc_function(func_name):
                            # Log skip reason and use default
                            log(f"[SKIP] {func_name} (libc) -> {DEFAULT_LIBC_SEARCHER}")
                            conn.send((DEFAULT_LIBC_SEARCHER + "\n").encode())
                            continue
                        
                        # ============ LOG: RECEIVED FROM KLEE ============
                        if query_type == 'health':
                            log(f"[HEALTH] {health_status} (in {func_name})")
                        else:
                            log(f"[QUERY] {func_name}")
                        
                        # Verbose: show all state features
                        if self.verbose:
                            log(f"  Signature: {features.get('signature', 'unknown')}")
                            for key, value in sorted(features.items()):
                                if key not in ('function', 'signature'):
                                    log(f"  {key}: {value}")
                        
                        # Query LLM (query_type already extracted above)
                        strategy, llm_raw_response, prompt = self.query_llm_with_logging(
                            features, schema, query_type)
                        
                        # Send response to KLEE
                        conn.send((strategy + "\n").encode())
                        
                        # ============ LOG: SENT TO KLEE ============
                        if query_type == 'health':
                            log(f"[RESPONSE] Health({health_status}) -> {strategy}")
                        else:
                            log(f"[RESPONSE] {func_name} -> {strategy}")
                            
                    except json.JSONDecodeError as e:
                        log(f"JSON PARSE ERROR: {e}")
                        log(f"  Raw data: {line[:200]}...")
                        conn.send(b"nurs:covnew\n")
                    except Exception as e:
                        log(f"ERROR: {e}")
                        import traceback
                        traceback.print_exc()
                        conn.send(b"nurs:covnew\n")
                    
        except (BrokenPipeError, ConnectionResetError):
            log("KLEE DISCONNECTED")
            log(f"Total queries processed: {self.query_count}")
        finally:
            conn.close()
    
    def query_llm_with_logging(self, features: dict, schema: dict, query_type: str = "function") -> tuple:
        """Query the LLM and return (strategy, raw_response, prompt)."""
        if query_type == "health":
            prompt = self._build_health_prompt(features, schema)
        else:
            prompt = self._build_prompt(features, schema)
        
        # ============ LOG: PROMPT SENT TO LLM (verbose only) ============
        if self.verbose:
            log("--- PROMPT ---")
            for line in prompt.split("\n"):
                log(f"  {line}")
            log("--- END PROMPT ---")
        
        try:
            if self.provider == "openai":
                raw_response = self._query_openai_raw(prompt)
            else:
                raw_response = self._query_anthropic_raw(prompt)
            
            strategy = self._parse_strategy(raw_response)
            
            # Always log the reasoning
            log(f"  Reasoning: {raw_response}")
            
            return strategy, raw_response, prompt
            
        except Exception as e:
            log(f"LLM API ERROR: {e}")
            return "nurs:covnew", f"ERROR: {e}", prompt
    
    def _query_openai_raw(self, prompt: str) -> str:
        """Query OpenAI API and return raw response."""
        messages = [
            {"role": "system", "content": self.system_prompt},
            {"role": "user", "content": prompt}
        ]
        response = self.llm_client.chat.completions.create(
            model=self.model,
            messages=messages,
            temperature=0.1,
            max_tokens=150  # Allow room for reasoning
        )
        return response.choices[0].message.content.strip()
    
    def _query_anthropic_raw(self, prompt: str) -> str:
        """Query Anthropic API and return raw response."""
        response = self.llm_client.messages.create(
            model=self.model,
            max_tokens=150,  # Allow room for reasoning
            system=self.system_prompt,
            messages=[{"role": "user", "content": prompt}]
        )
        return response.content[0].text.strip()
    
    def _parse_strategy(self, answer: str) -> str:
        """Extract searcher name from LLM response."""
        answer_lower = answer.lower().strip()
        
        # Try exact match first
        for s in SEARCHERS:
            if s == answer_lower:
                return s
        
        # Try substring match
        for s in SEARCHERS:
            if s in answer_lower:
                return s
        
        # Try partial matches for common variations
        if "covnew" in answer_lower or "coverage" in answer_lower:
            return "nurs:covnew"
        if "md2u" in answer_lower or "mindist" in answer_lower or "uncovered" in answer_lower:
            return "nurs:md2u"
        if "depth" in answer_lower:
            return "nurs:depth"
        if "random" in answer_lower and "path" in answer_lower:
            return "random-path"
        if "random" in answer_lower:
            return "random-state"
        if "dfs" in answer_lower or "depth-first" in answer_lower:
            return "dfs"
        if "bfs" in answer_lower or "breadth" in answer_lower:
            return "bfs"
        if "query" in answer_lower and "cost" in answer_lower:
            return "nurs:qc"
        if "icnt" in answer_lower or "instcount" in answer_lower:
            return "nurs:icnt"
        
        # Default if no match
        log(f"  WARNING: Could not parse '{answer}', defaulting to nurs:covnew")
        return "nurs:covnew"
    
    def _build_prompt(self, features: dict, schema: dict) -> str:
        """Build prompt for LLM."""
        
        # Schema section - explains what each field means
        schema_text = ""
        if schema:
            schema_text = "## Field Descriptions\n"
            for key, desc in schema.items():
                schema_text += f"- **{key}**: {desc}\n"
            schema_text += "\n"
        
        # Data section - actual values
        data_text = "## Current Execution State\n"
        for key, value in features.items():
            data_text += f"- {key}: {value}\n"
        
        return f"""You are a symbolic execution expert helping KLEE maximize code coverage.

{schema_text}{data_text}

## Available Searchers (choose one)
- **dfs**: Depth-first search. Go deep into execution paths. Best for sequential logic, parsers, interpreters, state machines.
- **bfs**: Breadth-first search. Explore all branches at current level first. Best for functions with many independent branches.
- **random-state**: Random state selection. Adds exploration diversity.
- **random-path**: Random path selection on execution tree. Good diversity, helps escape local minima.
- **nurs:covnew**: Coverage-new - prioritize states that recently covered new code. Best default for coverage.
- **nurs:md2u**: Min-distance-to-uncovered - prioritize states closest to uncovered code. Good for targeted coverage.
- **nurs:depth**: Prioritize by execution depth. Similar to DFS but with randomization.
- **nurs:rp**: Random priority - assigns random weights. Pure exploration.
- **nurs:icnt**: Instruction count - prioritize states with fewer instructions executed.
- **nurs:cpicnt**: Call-path instruction count - considers the entire call path.
- **nurs:qc**: Query cost - deprioritize states with expensive solver queries. Helps avoid timeouts.

## Your Task
Analyze the execution state and choose the BEST searcher to maximize code coverage efficiently.

Key factors to consider:
1. Function name/signature - what kind of function is this?
2. constraints count - high means complex paths, may need nurs:qc or simpler searcher
3. insts_since_cov_new - high means stuck, try random-path or random-state
4. covered_new - if true, this state is productive, keep exploring with dfs
5. min_dist_to_uncovered - low means close to new code, use nurs:md2u or nurs:covnew
6. active_states - many states means state explosion, may need to focus with dfs/bfs
7. solver_time - high means expensive queries, consider nurs:qc
8. **near_termination** - if true, this state is close to completing and generating a test case!
   - Use DFS or nurs:depth to push it to completion
9. **state_dist_to_return** - low value means this state is close to returning
   - Combined with low stack_depth, indicates imminent test generation
10. **completed_states** - if low relative to active_states, focus on completing paths (use DFS)

## Response Format
Respond in exactly this format:
REASONING: <1-2 sentences explaining your analysis>
CHOICE: <searcher name>"""

    def _build_health_prompt(self, features: dict, schema: dict) -> str:
        """Build prompt for health-based queries."""
        
        health_status = features.get('health_status', 'unknown')
        func_name = features.get('function', 'unknown')
        
        # Parse health issues
        issues = health_status.split(',') if health_status else []
        
        # Build issue explanation
        issue_explanations = []
        for issue in issues:
            if issue == 'coverage_stalled':
                issue_explanations.append("⚠️ **Coverage Stalled**: No new code has been covered for multiple check intervals. The current strategy may be stuck in a local minimum.")
            elif issue == 'state_explosion':
                issue_explanations.append(f"⚠️ **State Explosion**: {features.get('active_states', '?')} states are active. Too many parallel execution paths are being explored.")
            elif issue == 'solver_pressure':
                issue_explanations.append("⚠️ **Solver Pressure**: SMT solver is consuming >70% of execution time. Complex constraints are slowing exploration.")
            elif issue == 'memory_pressure':
                issue_explanations.append("⚠️ **Memory Pressure**: High constraint counts with many states indicate potential memory exhaustion.")
            elif issue == 'low_test_generation':
                issue_explanations.append(f"⚠️ **Low Test Generation**: {features.get('active_states', '?')} states but only {features.get('completed_states', '?')} tests generated. States are not completing - need to push paths to termination.")
        
        issues_text = "\n".join(issue_explanations) if issue_explanations else "Unknown health issue"
        
        # Key metrics
        metrics_text = f"""## Current Metrics
- Active States: {features.get('active_states', '?')}
- Completed States (ktests): {features.get('completed_states', '?')}
- Covered Instructions: {features.get('covered_instructions', '?')}
- Uncovered Instructions: {features.get('uncovered_instructions', '?')}
- Current Function: {func_name}
- Constraints: {features.get('constraints', '?')}
- Solver Time (μs): {features.get('solver_time_us', '?')}
- Instructions Since New Coverage: {features.get('insts_since_cov_new', '?')}
- Total Forks: {features.get('total_forks', '?')}
- State Distance to Return: {features.get('state_dist_to_return', '?')}
- Near Termination: {features.get('near_termination', '?')}"""

        return f"""You are a symbolic execution expert. KLEE has detected a health issue that requires strategy adjustment.

## Health Alert
{issues_text}

{metrics_text}

## Recommended Strategies by Issue

**Coverage Stalled:**
- `random-path` or `random-state`: Add randomness to escape local optima
- `nurs:md2u`: Target uncovered code directly

**State Explosion:**
- `dfs`: Focus on completing paths to reduce state count
- `bfs`: Systematically explore breadth before going deeper

**Solver Pressure:**
- `nurs:qc`: Deprioritize states with expensive queries
- `dfs`: Simpler paths often have simpler constraints

**Memory Pressure:**
- `dfs`: Complete and terminate states to free memory
- `nurs:icnt`: Favor states that have done less work

**Low Test Generation:**
- `dfs`: STRONGLY RECOMMENDED - drives paths to completion, generates ktests
- `nurs:depth`: Prioritize deep states that are close to finishing
- States need to COMPLETE to generate test cases (ktests)

## Your Task
Choose the BEST searcher to address the health issue(s) while maintaining good coverage progress.

## Response Format
REASONING: <1-2 sentences explaining why this addresses the health issue>
CHOICE: <searcher name>"""


def main():
    parser = argparse.ArgumentParser(description="KLEE Policy Server (LLM-powered)")
    parser.add_argument("--socket", default="/tmp/klee-policy.sock",
                        help="Unix socket path (default: /tmp/klee-policy.sock)")
    parser.add_argument("--provider", choices=["openai", "anthropic"], required=True,
                        help="LLM provider (required)")
    parser.add_argument("--model", help="LLM model name (default: gpt-4o-mini or claude-3-haiku)")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Verbose output (show full prompts)")
    parser.add_argument("--log", "-l", metavar="FILE",
                        help="Log file path (e.g., policy_server.log)")
    parser.add_argument("--system-prompt", "-s", metavar="FILE",
                        help="File containing custom system prompt (overrides default)")
    parser.add_argument("--program", "-p", metavar="NAME",
                        help="Name of program being analyzed (e.g., 'GNU Make', 'coreutils')")
    args = parser.parse_args()
    
    # Initialize log file if specified
    if args.log:
        init_log_file(args.log)
    
    # Load custom system prompt if specified
    system_prompt = None
    if args.system_prompt:
        try:
            with open(args.system_prompt, 'r') as f:
                system_prompt = f.read()
            log(f"Loaded system prompt from: {args.system_prompt}")
        except Exception as e:
            print(f"Error loading system prompt: {e}")
            sys.exit(1)
    
    # Check for API keys
    if args.provider == "openai":
        if not os.environ.get("OPENAI_API_KEY"):
            print("Error: OPENAI_API_KEY environment variable not set")
            print("Run: export OPENAI_API_KEY=sk-...")
            sys.exit(1)
    elif args.provider == "anthropic":
        if not os.environ.get("ANTHROPIC_API_KEY"):
            print("Error: ANTHROPIC_API_KEY environment variable not set")
            print("Run: export ANTHROPIC_API_KEY=...")
            sys.exit(1)
    
    try:
        server = PolicyServer(
            socket_path=args.socket,
            provider=args.provider,
            model=args.model,
            verbose=args.verbose,
            system_prompt=system_prompt,
            program=args.program
        )
        server.start()
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
