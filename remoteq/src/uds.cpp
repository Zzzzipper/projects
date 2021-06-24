#include <vector>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "loguru.h"
#include "uds.h"

/**
 * @brief recv_data - Read data from socket fd.
 * @param sockfd    - The socket fd
 * @param buf       - The buffer to keep data
 * @param len       - The length of buffer
 * @param flags     - Flags pass to recv() function
 * @return          - Bytes received.
 */
static ssize_t recv_data(int sockfd, char *buf, ssize_t len, int flags)
{
    ssize_t bytes;
    ssize_t pos;
    int count;
    fd_set readfds, writefds;
    struct timeval timeout;


    pos = 0;
    do {
        bytes = recv(sockfd, buf+pos, static_cast<size_t>(len-pos), flags);
        if (bytes < 0) {
            perror("recv error");
            break;
        } else if (bytes == 0) {
            /* No data left, jump out */
            break;
        } else {
            pos += bytes;
            if (pos >= len) {
                /* The buffer is full, jump out */
                break;
            }
        }

        /*
         * Check if data is available from socket fd, count is 0 when no data
         * available. Make select wait up to 10 ms for incoming data.
         * NOTES: On Linux, select() modifies timeout to reflect the amount of time
         * not slept.
         */
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(sockfd, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 10*1000;
        count = select(sockfd + 1, &readfds, &writefds, reinterpret_cast<fd_set*>(0), &timeout);
        if (count <= 0)  {
            break;
        }
    } while (count > 0);

    return pos;
}


#if 0
/**
 * @brief recv_packet   - Receive a command packet. Use the data_len field in the header to avoid
 *                        "no message boundaries" issue in "SOCK_STREAM" type socket.
 *                        (1) Receive the header of packet
 *                        (2) Check the signature of packet
 *                        (3) Get the data length of packet
 *                        (4) Receive the data of packet
 * @param sockfd        - The socket fd
 * @param buf           - The buffer to keep command packet
 * @param len           - The length of buffer
 * @param flags         - Flags pass to recv() function
 * @return              - Bytes received.
 */
static ssize_t recv_packet(int sockfd, char *buf, ssize_t len, int flags)
{
    ssize_t bytes;
    ssize_t pos;
    int count;
    fd_set readfds, writefds;
    struct timeval timeout;
    uds_command_t *pkt = (uds_command_t *)buf;
    ssize_t header_len = sizeof(uds_command_t);

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(sockfd, &readfds);

    /* Receive the header of command packet first */
    pos = 0;
    while (pos < header_len) {
        bytes = recv(sockfd, buf+pos, header_len-pos, flags);
        if (bytes < 0) {
            perror("recv error");
            return 0;
        } else if (bytes == 0) {
            /* No data left, jump out */
            return pos;
        } else {
            pos += bytes;
            if (pos >= header_len) {
                /* All data of header received, jump out */
                break;
            }
        }

        /*
         * Check if data is available from socket fd, count is 0 when no data
         * available.
         * Make select wait up to 10 ms for incoming data.
         * NOTES: On Linux, select() modifies timeout to reflect the amount of time
         * not slept.
         */
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(sockfd, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 10*1000;
        count = select(sockfd + 1, &readfds, &writefds, (fd_set *)0, &timeout);
        if (count <= 0)  {
            break;
        }
    }

    /* Check the signature of command packet */
    if (pkt->signature != UDS_SIGNATURE) {
        LOG_F(ERROR, "Invalid signature of packet");
        return 0;
    }

    /* Get the total length of command packet */
    if (len > pkt->data_len + header_len) {
        len = pkt->data_len + header_len;
    }

    /* Receive all data of command packet */
    while (pos < len) {
        bytes = recv(sockfd, buf+pos, len-pos, flags);
        if (bytes < 0) {
            perror("recv error");
            break;
        } else if (bytes == 0) {
            /* No data left, jump out */
            break;
        } else {
            pos += bytes;
            if (pos >= len) {
                /* All data received, jump out */
                break;
            }
        }

        /*
         * Check if data is available from socket fd, count is 0 when no data
         * available.
         * Make select wait up to 10 ms for incoming data.
         * NOTES: On Linux, select() modifies timeout to reflect the amount of time
         * not slept
         */
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(sockfd, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 10*1000;
        count = select(sockfd + 1, &readfds, &writefds, reinterpret_cast<fd_set*>(0), &timeout);
        if (count <= 0)  {
            break;
        }
    }

    return pos;
}
#endif

/**
 * @brief compute_checksum  - Compute 16-bit One's Complement sum of data. (The algorithm comes from
 *                            RFC-1071)
 *                            NOTES: Before call this function, please set the checksum field to 0.
 * @param buf               - The data buffer
 * @param len               - The length of data(bytes).
 * @return                  - Checksum
 */
static uint16_t compute_checksum(void *buf, ssize_t len)
{
    uint16_t *word;
    uint8_t *byte;
    ssize_t i;
    unsigned long sum = 0;

    if (!buf) {
        return 0;
    }

    word = reinterpret_cast<uint16_t *>(buf);
    for (i = 0; i < len/2; i++) {
        sum += word[i];
    }

    /* If the length(bytes) of data buffer is an odd number, add the last byte. */
    if (len & 1) {
        byte = reinterpret_cast<uint8_t*>(buf);
        sum += byte[len-1];
    }

    /* Take only 16 bits out of the sum and add up the carries */
    while (sum>>16) {
        sum = (sum>>16) + (sum&0xFFFF);
    }

    return static_cast<uint16_t>(~sum);
}


/**
 * @brief verify_command_packet - Verify the data integrity of the command packet.
 * @param buf                   - The data of command packet
 * @param len                   - The length of data
 * @return                      1 - OK, 0 - FAIL
 */
static int verify_command_packet(void *buf, size_t len)
{
    uds_command_t *pkt;

    if (buf == nullptr) {
        return 0;
    }
    pkt = reinterpret_cast<uds_command_t*>(buf);

    if (pkt->signature != UDS_SIGNATURE) {
        LOG_F(ERROR, "Invalid signature of packet (0x%08X)", pkt->signature);
        return 0;
    }

    if (pkt->data_len + sizeof(uds_command_t) != len) {
        LOG_F(ERROR, "Invalid length of packet (%ld:%ld)",
               pkt->data_len + sizeof(uds_command_t), len);
        return 0;
    }

    if (compute_checksum(buf, static_cast<ssize_t>(len)) != 0) {
        LOG_F(ERROR, "Invalid checksum of packet");
        return 0;
    }

    return 1;
}


/**
 * @brief request_handle_routine    - A thread function to receive the request from client,
 *                                    handle the request and send response back to client.
 * @param arg                       - A pointer of connection info
 * @return                          - None
 */
static void *request_handle_routine(void *arg)
{
    uds_connect_t *sc = reinterpret_cast<uds_connect_t*>(arg);
    uds_command_t *req;
    uds_command_t *resp;
    uint8_t buf[UDS_BUF_SIZE];
    ssize_t bytes, req_len, resp_len;

    if (sc == nullptr) {
        LOG_F(ERROR, "Invalid argument of thread routine");
        pthread_exit(nullptr);
    }

    while (1) {
        /* Receive request from client */
        //req_len = recv(sc->client_fd, buf, sizeof(buf), 0);
        req_len = recv_data(sc->client_fd, reinterpret_cast<char *>(buf), sizeof(buf), 0);
        if (req_len <= 0) {
            close(sc->client_fd);
            sc->inuse = 0;
            break;
        }

        /* Check the integrity of the request packet */
        if (!verify_command_packet(buf, static_cast<size_t>(req_len))) {
            /* Discard invaid packet */
            continue;
        }

        /* Process the request */
        req = reinterpret_cast<uds_command_t*>(buf);
        resp = sc->serv->request_handler(req);
        if (resp == nullptr) {
            resp = reinterpret_cast<uds_command_t*>(buf);   /* Use a local buffer */
            resp->status = STATUS_ERROR;
            resp->data_len = 0;
        }

        resp_len = sizeof(uds_command_t) + resp->data_len;
        resp->signature = req->signature;
        resp->checksum = 0;
        resp->checksum = compute_checksum(resp, resp_len);

        /* Send response */
        bytes = send(sc->client_fd, resp, static_cast<size_t>(resp_len), MSG_NOSIGNAL);
        if (resp != reinterpret_cast<uds_command_t*>(buf)) {    /* If NOT local buffer, free it */
            free(resp);
        }
        if (bytes != resp_len) {
            LOG_F(ERROR, "Send response error");
            close(sc->client_fd);
            sc->inuse = 0;
            break;
        }
    }

    pthread_exit(nullptr);
}


/**
 * @brief server_init   - Do some initialzation work for server.
 * @param sock_path     - The path of unix domain socket
 * @param req_handler   - The function pointer of a user-defined request handler.
 * @return              - A pointer of server info.
 */
uds_server_t *server_init(const char *sock_path, request_handler_t req_handler)
{
    uds_server_t *s;
    struct sockaddr_un addr;
    int i, rc;

    if (req_handler == nullptr) {
        LOG_F(ERROR, "Invalid parameter!");
        return nullptr;
    }

    s = (uds_server_t *)malloc(sizeof(uds_server_t));
    if (s == nullptr) {
        perror("malloc error");
        return nullptr;
    }
    memset(s, 0, sizeof(uds_server_t));
    for (i = 0; i < UDS_MAX_CLIENT; i++) {
        s->conn[i].serv = s;
    }

    /* Setup request handler */
    s->request_handler = req_handler;

    unlink(sock_path);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, sock_path);

    s->sockfd = socket(AF_UNIX, UDS_SOCK_TYPE, 0);
    if (s->sockfd < 0) {
        perror("socket error");
        free(s);
        return nullptr;
    }

    /* Avoid "Address already in use" error in bind() */
    //int val = 1;
    //if (setsockopt(s->sockfd, SOL_SOCKET, SO_REUSEADDR, &val,
    //        sizeof(val)) == -1) {
    //    perror("setsockopt error");
    //    return NULL;
    //}

    rc = bind(s->sockfd, (struct sockaddr *) &addr, sizeof(addr));
    if (rc != 0) {
        perror("bind error");
        close(s->sockfd);
        free(s);
        return nullptr;
    }

    rc = listen(s->sockfd, UDS_MAX_BACKLOG);
    if (rc != 0) {
        perror("listen error");
        close(s->sockfd);
        free(s);
        return nullptr;
    }

    return s;
}

/**
 * @brief server_accept_request - Accept a request from client and start a new thread to process it.
 * @param s                     - A pointer of server info
 * @return                      - 0 - OK, Others - Error
 */
int server_accept_request(uds_server_t *s)
{
    uds_connect_t *sc;
    int cl, i;

    if (s == nullptr) {
        LOG_F(ERROR, "Invalid parameter!");
        return -1;
    }

    cl = accept(s->sockfd, nullptr, nullptr);
    if (cl < 0) {
        perror("accept error");
        return -1;
    }

    /* Find a slot for the connection */
    for (i = 0; i < UDS_MAX_CLIENT; i++) {
        if (!s->conn[i].inuse) {
            break;
        }
    }
    if (i >= UDS_MAX_CLIENT) {
        LOG_F(ERROR, "Too many connections");
        close(cl);
        return -1;
    }

    /* Start a new thread to handle the request */
    sc = &s->conn[i];
    sc->inuse = 1;
    sc->client_fd = cl;
    if (pthread_create(&sc->thread_id, nullptr, request_handle_routine, sc) != 0) {
        perror("pthread_create error");
        close(cl);
        sc->inuse = 0;
        return -1;
    }

    return 0;
}


/**
 * @brief server_close  - Close the socket fd and free memory.
 * @param s             - A pointer of server info
 */
void server_close(uds_server_t *s)
{
    int i;

    LOG_F(INFO, "Server closing");

    if (s == nullptr) {
        return;
    }

    for (i = 0; i < UDS_MAX_CLIENT; i++) {
        if (s->conn[i].inuse) {
            pthread_join(s->conn[i].thread_id, nullptr);
            close(s->conn[i].client_fd);
        }
    }

    close(s->sockfd);
    free(s);
}


/**
 * @brief client_init   - Init client and connect to the server
 * @param sock_path     - The path of unix domain socket
 * @param timeout       - Wait the server to be ready(in seconds)
 * @return A pointer of client info.
 */
uds_client_t *client_init(const char *sock_path, int timeout)
{
    uds_client_t *sc;
    struct sockaddr_un addr;
    int fd, rc;

    sc = reinterpret_cast<uds_client_t*>(malloc(sizeof(uds_client_t)));
    if (sc == nullptr) {
        perror("malloc error");
        return nullptr;
    }
    memset(sc, 0, sizeof(uds_client_t));

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, sock_path);
    fd = socket(AF_UNIX, UDS_SOCK_TYPE, 0);
    if (fd < 0) {
        perror("socket error");
        free(sc);
        return nullptr;
    }
    sc->sockfd = fd;

    do {
        rc = connect(sc->sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
        if (rc == 0) {
            break;
        } else {
            sleep(1);
        }
    } while (timeout-- > 0);
    if (rc != 0) {
        perror("connect error");
        close(sc->sockfd);
        free(sc);
        return nullptr;
    }

    return sc;
}

/**
 * @brief client_send_request   - Send a request to server, and get the response.
 * @param c                     - A pointer of client info
 * @param req                   - The request to send
 * @return - The response for the request. The caller need to free the memory.
 */
uds_command_t *client_send_request(uds_client_t *c, uds_command_t *req)
{
    uint8_t buf[UDS_BUF_SIZE];
    ssize_t bytes, req_len;

    if ((c == nullptr) || (req == nullptr)) {
        LOG_F(ERROR, "Invalid parameter!");
        return nullptr;
    }

    /* Send request */
    req_len = sizeof(uds_command_t) + req->data_len;
    req->signature = UDS_SIGNATURE;
    req->checksum = 0;
    req->checksum = compute_checksum(req, req_len);
    bytes = send(c->sockfd, req, static_cast<size_t>(req_len), MSG_NOSIGNAL);
    if (bytes != req_len) {
        perror("send error");
        return nullptr;
    }

    /* Get response */
    //bytes = recv(c->sockfd, buf, sizeof(buf), 0);
    bytes = recv_data(c->sockfd, reinterpret_cast<char*>(buf), sizeof(buf), 0);
    if (bytes <= 0) {
        LOG_F(ERROR, "Receive response error");
        return nullptr;
    }

    if (verify_command_packet(buf, static_cast<size_t>(bytes))) {
        uds_command_t *resp = reinterpret_cast<uds_command_t*>(malloc(static_cast<size_t>(bytes)));
        if (resp) {
            memcpy(resp, buf, static_cast<size_t>(bytes));
        } else {
            perror("malloc error");
        }
        return resp;
    }

    return nullptr;
}


/**
 * @brief client_close  - Close the client socket fd and free memory.
 * @param c             - The pointer of client info
 */
void client_close(uds_client_t *c)
{
    if (c == nullptr) {
        return;
    }
    close(c->sockfd);
    free(c);
}
