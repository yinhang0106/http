#include <netdb.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include <fmt/format.h>

struct socket_address_fatprt {
    struct sockaddr *m_addr;
    socklen_t m_addrlen;
};

struct socket_address_storage {
    union {
        struct sockaddr m_addr{};
        struct sockaddr_storage m_addr_storage;
    };
    socklen_t m_addrlen = sizeof(m_addr_storage);

    explicit operator socket_address_fatprt() {
        return {&m_addr, m_addrlen};
    }
};


int main() {
    struct addrinfo *addrinfo;
    int err = getaddrinfo("127.0.0.1", "8080", nullptr, &addrinfo);
    if (err != 0) {
        fmt::print("getaddrinfo: {}\n", gai_strerror(err));
        return 1;
    }

    int sockfd = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
    if (sockfd == -1) {
        fmt::print("socket: {}\n", strerror(errno));
        return 1;
    }

    int res = bind(sockfd, addrinfo->ai_addr, addrinfo->ai_addrlen);
    if (res == -1) {
        fmt::print("bind: {}\n", strerror(errno));
        return 1;
    }

    int listenid = listen(sockfd, SOMAXCONN);
    if (listenid == -1) {
        fmt::print("listen: {}\n", strerror(errno));
        return 1;
    }

    while (true) {
        socket_address_storage addr_storage;
        int connid = accept(sockfd, &addr_storage.m_addr, &addr_storage.m_addrlen);
        if (connid == -1) {
            fmt::print("accept: {}\n", strerror(errno));
            return 1;
        }
        fmt::print("Accepted connection\n");
        std::thread([connid] {
            char buf[1024];
            size_t n = read(connid, buf, sizeof(buf));
            auto req = std::string_view(buf, n);
            fmt::print("Received: {}\n", req);
            auto res = "hello, " + std::string(req);
            auto w = write(connid, res.data(), res.size());
            close(connid);
        });
    }
    freeaddrinfo(addrinfo);


    return 0;
}
