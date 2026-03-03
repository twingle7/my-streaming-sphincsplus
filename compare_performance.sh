#!/bin/bash

# Performance Comparison Script
# This script helps compare performance between the original (main) and modified (modify-r-generation) branches

echo "================================================================"
echo "  SPHINCS+ Performance Comparison Script"
echo "================================================================"
echo ""

# Get current branch
CURRENT_BRANCH=$(git branch --show-current)
echo "Current branch: $CURRENT_BRANCH"
echo ""

# Function to build and test on a branch
test_branch() {
    local branch=$1
    local output_file=$2

    echo "================================================================"
    echo "Testing branch: $branch"
    echo "================================================================"

    # Checkout the branch
    git checkout "$branch" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to checkout branch $branch"
        return 1
    fi

    echo "Building..."
    make clean > /dev/null 2>&1
    make test_performance_orig > /dev/null 2>&1

    if [ ! -f build/test_performance_orig ]; then
        echo "ERROR: Failed to build test_performance_orig on branch $branch"
        return 1
    fi

    echo "Running performance test..."
    build/test_performance_orig > "$output_file"

    if [ $? -eq 0 ]; then
        echo "Test completed successfully. Results saved to $output_file"
    else
        echo "ERROR: Test failed on branch $branch"
        return 1
    fi

    echo ""
    return 0
}

# Check if we want to test both branches or just current
if [ "$1" == "--compare" ]; then
    # Test both branches
    echo "Running comparison between main and modify-r-generation branches"
    echo ""

    # Save current branch
    ORIG_BRANCH=$CURRENT_BRANCH

    # Test main branch
    test_branch "main" "results_main.txt"
    MAIN_RESULT=$?

    # Test modify-r-generation branch
    test_branch "modify-r-generation" "results_modified.txt"
    MODIFIED_RESULT=$?

    # Return to original branch
    git checkout "$ORIG_BRANCH" > /dev/null 2>&1

    # Check if both tests succeeded
    if [ $MAIN_RESULT -eq 0 ] && [ $MODIFIED_RESULT -eq 0 ]; then
        echo "================================================================"
        echo "  Comparison Complete"
        echo "================================================================"
        echo ""
        echo "Results saved to:"
        echo "  - results_main.txt (original version)"
        echo "  - results_modified.txt (streaming version)"
        echo ""
        echo "To compare the results, use:"
        echo "  diff results_main.txt results_modified.txt"
        echo ""
    else
        echo "ERROR: One or both tests failed"
        exit 1
    fi
else
    # Just test current branch
    echo "Testing current branch only"
    echo ""

    echo "Building..."
    make clean > /dev/null 2>&1
    make test_performance_orig > /dev/null 2>&1

    if [ ! -f build/test_performance_orig ]; then
        echo "ERROR: Failed to build test_performance_orig"
        exit 1
    fi

    echo "Running performance test..."
    build/test_performance_orig

    if [ $? -eq 0 ]; then
        echo ""
        echo "================================================================"
        echo "  Test Complete"
        echo "================================================================"
    else
        echo "ERROR: Test failed"
        exit 1
    fi
fi
