for f in ../testcases/*.tig; do
    ./lextest "$f" > /dev/null || echo "FAILED: $f"
done
