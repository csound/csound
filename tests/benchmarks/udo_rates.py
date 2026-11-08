"""Generate an orchestra for the UDO rate-classification compile benchmark.

Write stdout to an .orc file and pass it to compile_expression_args.py --source.
The definitions are compiled but never called, so large recursive groups are safe.
"""
import argparse


def orchestra(count, graph, order, performance):
    lines = [f"declare RateNode{i}(input:i):(i)" for i in range(count)]
    indices = range(count) if order == "caller-first" else reversed(range(count))
    for i in indices:
        lines.append(f"opcode RateNode{i}(input:i):i")
        if graph == "branching":
            children = [n for n in (2*i+1, 2*i+2) if n < count]
        else:
            children = [i+1] if i+1 < count else ([0] if graph == "cycle" else [])
        if performance and i == count-1:
            lines.append('printks "", 1')
        for index, child in enumerate(children):
            lines.append(f"result{index}:i = RateNode{child}(input)")
        result = " + ".join(f"result{n}" for n in range(len(children))) or "input"
        lines.extend([f"xout {result}", "endop"])
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--count", type=int, default=256)
    parser.add_argument("--graph", choices=("chain", "branching", "cycle"), default="chain")
    parser.add_argument("--order", choices=("caller-first", "callee-first"), default="caller-first")
    parser.add_argument("--init-only", action="store_true")
    args = parser.parse_args()
    if args.count < 1:
        parser.error("count must be positive")
    print(orchestra(args.count, args.graph, args.order, not args.init_only), end="")
