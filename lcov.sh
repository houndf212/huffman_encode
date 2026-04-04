lcov -c --branch-coverage -d . -o cover.info
genhtml cover.info --branch-coverage -o cover
