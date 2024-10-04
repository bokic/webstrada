#include <iostream>

#include <linux/prctl.h>  /* Definition of PR_* constants */
#include <sys/prctl.h>
#include <signal.h>

#include <unistd.h>


using namespace std;

int main()
{
    signal(SIGCHLD, SIG_IGN);

    prctl(PR_SET_NAME, "Boro123");
    //pthread_setname_np(pthread_self(), "Boro");

    cout << "Hello World!" << endl;


    if (fork() == 0) {
        cout << "Hello World2!" << endl;
        //prctl(PR_SET_NAME, "Boro2");
        pthread_setname_np(pthread_self(), "Boro2");
        sleep(5);
        _exit(0);
    }

    sleep(10);
    return 0;
}
