#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>

#include <fmt/format.h>

int check_error(std::string const& msg, int res) {
    if (res == -1) {
        fmt::print("{}: {}\n", msg, strerror(errno));
        throw;
    }
    return res;
}

ssize_t check_error(std::string const& msg, ssize_t res) {
    if (res == -1) {
        fmt::print("{}: {}\n", msg, strerror(errno));
        throw;
    }
    return res;
}
#define CHECK_CALL(func, ...) check_error(#func, func(__VA_ARGS__))

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

struct address_resolved_entry {
    struct addrinfo *m_curr = nullptr;

    [[nodiscard]]
    socket_address_fatprt get_address() const {
        return {m_curr->ai_addr, m_curr->ai_addrlen};
    }

    [[nodiscard]]
    int create_socket() const {
        int sockfd = CHECK_CALL(socket, m_curr->ai_family, m_curr->ai_socktype, m_curr->ai_protocol);
        return sockfd;
    }
};

struct address_resolver {
    struct addrinfo *m_head = nullptr;

    address_resolved_entry resolve(std::string const& name, std::string const& port) {
        int err = getaddrinfo(name.c_str(), port.c_str(), nullptr, &m_head);
        if (err != 0) {
            fmt::print("getaddrinfo: {} : {}\n", err, gai_strerror(err));
            throw;
        }
        return {m_head};
    }

    address_resolver() = default;

    address_resolver(address_resolver &&that)  noexcept : m_head(that.m_head) {
        that.m_head = nullptr;
    }

    ~address_resolver() {
        if (m_head) {
            freeaddrinfo(m_head);
        }
    }
};

std::vector<std::thread> pool;

int main() {

    setlocale(LC_ALL, "en_US.UTF-8");
    address_resolver resolver;
    auto entry = resolver.resolve("127.0.0.1", "8080");
    int sockfd = entry.create_socket();
    auto addr = entry.get_address();
    int listenid = CHECK_CALL(bind, sockfd, addr.m_addr, addr.m_addrlen);
    CHECK_CALL(listen, listenid, SOMAXCONN);
    while (true) {
        socket_address_storage peer_addr;
        int connid = CHECK_CALL(accept, sockfd, &peer_addr.m_addr, &peer_addr.m_addrlen);
        pool.emplace_back([connid] {
            char buf[1024];
            size_t n = CHECK_CALL(read, connid, buf, sizeof(buf));
            auto req = std::string_view(buf, n);
            fmt::print("Received: {}\n", req);
            auto res = "hello, " + std::string(req);
            CHECK_CALL(write, connid, res.data(), res.size());
            close(connid);
        });
    }
    for (auto &t : pool) {
        t.join();
    }
    return 0;
}