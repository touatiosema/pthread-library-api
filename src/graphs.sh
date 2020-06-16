MAX_TH=500
MAX_TH_SWITCH=200
MAX_FIBO=20
NB_RUNS=5
rm graph*;

libthread21=(); libthread22=(); libthread23=();
libthread31=(); libthread32=(); libthread51=();
pthread21=(); pthread22=(); pthread23=();
pthread31=(); pthread32=(); pthread51=();

for i in $(seq 1 $MAX_TH)
do
    for j in $(seq 1 $NB_RUNS)
    do
        libthread21+=( $(./build/21-create-many $i | cut -d " " -f 8) )
        pthread21+=( $(numactl --physcpubind=3 ./build/21-create-many-pthread $i | cut -d " " -f 8) )
        libthread22+=( $(./build/22-create-many-recursive $i | cut -d " " -f 8) )
        pthread22+=( $(numactl --physcpubind=3 ./build/22-create-many-recursive-pthread $i | cut -d " " -f 8 ) )
        libthread23+=( $(./build/23-create-many-once $i | cut -d " " -f 10) ) 
        pthread23+=( $(numactl --physcpubind=3 ./build/23-create-many-once-pthread $i | cut -d " " -f 10) )
    done
    echo ${libthread21[@]} >> graph-21
    echo ${pthread21[@]} >> graph-pthread-21
    echo ${libthread22[@]} >> graph-22
    echo ${pthread22[@]} >> graph-pthread-22
    echo ${libthread23[@]} >> graph-23
    echo ${pthread23[@]} >> graph-pthread-23
    libthread21=(); libthread22=(); libthread23=();
    pthread21=(); pthread22=(); pthread23=();
done
for i in $(seq 1 $MAX_TH_SWITCH)
do
    for j in $(seq 1 $NB_RUNS)
    do
        libthread31+=( $(./build/31-switch-many $i $((i * 2)) | cut -d " " -f 6) )
        pthread31+=( $(numactl --physcpubind=3 ./build/31-switch-many-pthread $i $((i * 2)) | cut -d " " -f 6) )
        libthread32+=( $(./build/32-switch-many-join $i $((i * 2)) | cut -d " " -f 9) )
        pthread32+=( $(numactl --physcpubind=3 ./build/32-switch-many-join-pthread $i $((i * 2)) | cut -d " " -f 9) )
    done
    echo ${libthread31[@]} >> graph-31
    echo ${pthread31[@]} >> graph-pthread-31
    echo ${libthread32[@]} >> graph-32
    echo ${pthread32[@]} >> graph-pthread-32
    libthread31=(); libthread32=();
    pthread31=(); pthread32=();
done
for i in $(seq 1 $MAX_FIBO)
do
    for j in $(seq 1 $NB_RUNS)
    do
        libthread51+=( $(./build/51-fibonacci $i | cut -d " " -f 7 ) )
        pthread51+=( $(numactl --physcpubind=3 ./build/51-fibonacci-pthread $i | cut -d " " -f 7) )
    done
    echo ${libthread51[@]} >> graph-51
    echo ${pthread51[@]} >> graph-pthread-51
    libthread51=();
    pthread51=();
done
