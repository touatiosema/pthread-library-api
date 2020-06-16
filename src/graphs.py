import matplotlib.pyplot as plt
def avg(L):
    l = L.split()
    l = list(map(int, l))
    return sum(l) / len(l)

def avgf(L):
    l = L.split()
    l = list(map(float, l))
    return sum(l) / len(l)


files=["21", "22", "23", "31", "32", "51"]
libthread=[]
pthread=[]
for i in files:
    f= open("graph-" + i,"r")
    libthread.append(f.read());
    f.close()
    f= open("graph-pthread-" + i,"r")
    pthread.append(f.read());
    f.close()

for i in range(len(libthread)):
    libthread[i] = list(map(avg if (i != 5) else avgf, (libthread[i].split('\n'))[:-1]))
    pthread[i] = list(map(avg if (i != 5) else avgf, (pthread[i].split('\n'))[:-1]))


titles = ["21-create-many", "22-create-many-recursive", "23-create-many-once",
 "31-switch-many yields = 2x nb_th", "32-switch-many-join yields = 2x nb_th",
 "51-fibonacci"]
for i in range(len(files)):
    plt.figure()
    plt.plot(range(1, len(libthread[i]) + 1), libthread[i], 'r', label='libthread')
    plt.plot(range(1, len(pthread[i]) + 1), pthread[i], 'b', label='pthread')
    if(i != 5):
        plt.ylabel('time in us')
        plt.xlabel('number of threads')
    else :
        plt.ylabel('time in s')
        plt.xlabel('n')
    plt.suptitle(titles[i])
    plt.legend()
    plt.savefig(titles[i])

# plt.show()