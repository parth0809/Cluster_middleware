#!/bin/bash
set -e

# Kill hanging processes just in case
pkill -f master_app || true
pkill -f backup_app || true
pkill -f compute_app || true

# Build the object files first via standard Make-like compilation
echo "Compiling Client..."
cd ./client && g++ -std=c++14 -c jobs/job.cpp -o job.o
g++ -std=c++14 -c jobs/job_loading.cpp -o job_loading.o
g++ -std=c++14 client.cpp job.o job_loading.o -o client_app -pthread
cd ..

echo "Compiling Master & Backup Nodes..."
cd ./master && g++ -std=c++14 -c jobs/job.cpp -o job.o
g++ -std=c++14 -c jobs/job_forward.cpp -o job_forward.o
g++ -std=c++14 master.cpp job.o job_forward.o -o master_app -lsqlite3 -pthread
g++ -std=c++14 backup.cpp job.o job_forward.o -o backup_app -lsqlite3 -pthread
cd ..

echo "Compiling Compute..."
cd ./compute && g++ -std=c++14 -c jobs/job.cpp -o job.o
g++ -std=c++14 -c jobs/job_execution.cpp -o job_exec.o
g++ -std=c++14 compute.cpp job.o job_exec.o -o compute_app -lsqlite3 -pthread
cd ..

echo "Compilation successful!"

echo "To test high-availability failover pipeline:"
echo "1. Run terminal 1: cd compute && ./compute_app"
echo "2. Run terminal 2: cd master && ./backup_app"
echo "3. Run terminal 3: cd master && ./master_app"
echo "4. Run terminal 4: cd client && ./client_app"
