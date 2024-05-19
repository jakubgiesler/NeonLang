[ ! -d "FlameGraph" ] && git clone https://github.com/brendangregg/FlameGraph.git

perf record -g ./build/neonc main.n
perf script > trace.perf
FlameGraph/stackcollapse-perf.pl trace.perf > trace.folded
FlameGraph/flamegraph.pl trace.folded > trace.svg
firefox trace.svg