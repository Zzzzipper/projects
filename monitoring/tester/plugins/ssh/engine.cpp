#include <libssh/libssh.h>

#include "log.h"
#include "scope.h"

#include <boost/asio.hpp>
#include <boost/regex.hpp>

#include <string>

using boost::asio::ip::tcp;

template <class F>
static int SshCommand(const char* addr, const char* user, const char* password, F func) {
  try {
    boost::asio::io_context io_context;

    // Get a list of endpoints corresponding to the server name.
    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints = resolver.resolve(addr, "7222");
    if (endpoints.empty()) {
      LOG_ERROR << "Cannot resolve address " << addr;
      return 1;
    }
    ssh_session session = ::ssh_new();

    DEFER({
      ::ssh_free(session);
    });

    auto&& endpoint = endpoints.begin()->endpoint();
    int verbosity = SSH_LOG_NOLOG;
    int port = endpoint.port();
    ::ssh_options_set(session, SSH_OPTIONS_HOST, endpoint.address().to_string().c_str());
    ::ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
    ::ssh_options_set(session, SSH_OPTIONS_PORT, &port);
    int rc = ::ssh_connect(session);
    if (rc != SSH_OK) {
      LOG_ERROR << "Cannot connect " << ::ssh_get_error(session);
      return 1;
    }
    DEFER({
      ::ssh_disconnect(session);
    });

    LOG_DEBUG << "Connected";

    rc = ::ssh_userauth_password(session, user, password);
    if (rc != SSH_OK) {
      LOG_DEBUG << "Cannot authenticate";
      return rc;
    }
    LOG_DEBUG << "Authenticated";
    return func(session);
  } catch (const std::exception& ex) {
    LOG_ERROR << "Exception: " << ex.what();
    return 1;
  }
}

template <class F>
static int SshExecute(const char* addr, const char* user, const char* password, F func) {
  return SshCommand(addr, user, password, [&](ssh_session session) -> int {
    ssh_channel channel = ssh_channel_new(session);
    if (nullptr == channel) {
      LOG_ERROR << "Cannot open channel: " << ::ssh_get_error(session);
      return SSH_ERROR;
    }
    DEFER({
      ::ssh_channel_free(channel);
    });

    int rc = ::ssh_channel_open_session(channel);
    if (rc != SSH_OK) {
      LOG_ERROR << "Cannot open session: " << ::ssh_get_error(session);
      return rc;
    }
    DEFER({
      ::ssh_channel_close(channel);
    });
    return func(channel);
  });
}


int sshAuthenticate(const char* addr, const char* user, const char* password)
{
  return SshCommand(addr, user, password, [](ssh_session session) -> int {
    return 0;
  });
}

int sshCheckCall(const char* addr, const char* user, const char* password, const char* command, int code)
{
  return SshExecute(addr, user, password, [&](ssh_channel channel) -> int {
    LOG_DEBUG << "Execute command " << command;
    int rc = ::ssh_channel_request_exec(channel, command);
    if (rc != SSH_OK) {
      LOG_ERROR << "Cannot execute command: " << ::ssh_get_error(channel);
      return rc;
    }
    rc = ::ssh_channel_get_exit_status(channel);
    if (code != rc) {
      LOG_ERROR << "Expected code mismatch: " << code  << " != " << rc;
      return 1;
    }
    ::ssh_channel_send_eof(channel);
    return 0;
  });
}

int sshCheckOutput(const char* addr, const char* user, const char* password, const char* command, const char* regex)
{
  return SshExecute(addr, user, password, [&](ssh_channel channel) -> int {
    LOG_DEBUG << "Execute command " << command;
    int rc = ::ssh_channel_request_exec(channel, command);
    if (rc != SSH_OK) {
      LOG_ERROR << "Cannot execute command: " << ::ssh_get_error(channel);
      return rc;
    }
    rc = ssh_channel_request_exec(channel, "ps aux");
    if (rc != SSH_OK)
    {
      ssh_channel_close(channel);
      ssh_channel_free(channel);
      return rc;
    }
    std::string output;
    char buffer[256];
    int nbytes = ::ssh_channel_read(channel, buffer, sizeof(buffer), 0);
    while (nbytes > 0) {
      output.append(buffer, nbytes);
      nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
    }
    ::ssh_channel_send_eof(channel);
    if (!boost::regex_match(output, boost::regex(regex))) {
      LOG_ERROR << "Output does not match:" << output;
      return SSH_ERROR;
    }
    return SSH_OK;
  });
}

TLogLevel Log::reportingLevel = DEBUG;

#if 0

int main(int argc, char* argv[])
{
    try
    {
        if (int code = sshCheckOutput("91.109.201.27", "root", "86588fdfe1d7", "echo 1", "1")) {
          return code;
        }
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    return 0;
}

#endif
