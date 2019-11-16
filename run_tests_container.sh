sysbench --test=cpu --cpu-max-prime=20000 run > results/my_container_cpu.txt
sysbench --test=fileio --file-total-size=50G prepare
sysbench --test=fileio --file-total-size=50G --file-test-mode=rndwr --max-time=300 --init-rng=on --max-requests=0 run > results/my_container_fileio_wr.txt
sysbench --test=fileio --file-total-size=50G --file-test-mode=rndrd --max-time=300 --init-rng=on --max-requests=0 run > results/my_container_fileio_rd.txt
sysbench --test=fileio --file-total-size=50G cleanup
sysbench --test=memory run > results/my_container_memory.txt
sysbench --test=threads --num-threads=64 --thread-yields=1000 --thread-locks=8 run > results/my_container_threads.txt
