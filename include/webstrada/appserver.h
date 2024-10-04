#pragma once


namespace webstrada
{

class appserver
{
public:
    appserver();
    ~appserver();
    void init(const char *socket_name, int backlog, int worker_num);
    int run();
    int main();

private:
    int m_sock_fd = 0;
    int m_worker_num = 0;
};

}
