for f in ../testcases/*.tig; do
    ./a.out "$f" > /dev/null || echo "FAILED: $f"
done
