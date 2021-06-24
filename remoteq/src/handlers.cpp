#include "common.h"
#include "message.h"
#include "loguru.h"
#include "headmeters.h"

/**
 * @brief getCmdAsString - for debug print from handlers
 * @param cmd_
 * @return
 */
const char* getCmdAsString(uds_command_type cmd_) {
    switch(cmd_) {
    case CMD_GET_VERSION:   return "CMD_GET_VERSION";
    case CMD_GET_MESSAGE:   return "CMD_GET_MESSAGE";
    case CMD_PUT_MESSAGE:   return "CMD_PUT_MESSAGE";
    case CMD_START_MEASURE: return "CMD_START_MEASURE";
    case CMD_SET_RANGE:     return "CMD_SET_RANGE";
    case CMD_STOP_MEASURE:  return "CMD_STOP_MEASURE";
    case CMD_GET_STATUS:    return "CMD_GET_STATUS";
    case CMD_GET_RESULT:    return "CMD_GET_RESULT";
    case CMD_CHANNELS_INFO: return "CMD_CHANNELS_INFO";
    case CMD_UNKNOWN:       return "CMD_UNKNOWN";

    }
    return "<...>";
}

/*
 * Return the version of server.
 */
uds_command_t *cmd_get_version(void) {
    uds_response_version_t *ver;

    LOG_F(INFO, "CMD_GET_VERSION");

    ver = reinterpret_cast<uds_response_version_t*>(malloc(sizeof(uds_response_version_t)));
    if (ver != nullptr) {
        ver->common.status = STATUS_SUCCESS;
        ver->common.data_len = 2;
        ver->major = 1;
        ver->minor = 0;
    }

    return reinterpret_cast<uds_command_t*>(ver);
}


/*
 * Get a message string from server
 */
uds_command_t *cmd_get_msg(void) {
    uds_response_get_msg_t *res;
    const char *str = "This is a message from the server.";

    LOG_F(INFO, "CMD_GET_MESSAGE");

    res = reinterpret_cast<uds_response_get_msg_t *>(malloc(sizeof(uds_response_get_msg_t)));
    if (res != nullptr) {
        res->common.status = STATUS_SUCCESS;
        res->common.data_len = static_cast<uint32_t>(strlen(str));
        snprintf(res->data, UDS_GET_MSG_SIZE, "%s", str);
        res->data[UDS_GET_MSG_SIZE-1] = 0;
    }

    return reinterpret_cast<uds_command_t*>(res);
}


/*
 * Send a message string to server
 */
uds_command_t *cmd_put_msg(uds_command_t *req) {
    uds_command_t *res;
    uds_request_put_msg_t *put_msg = reinterpret_cast<uds_request_put_msg_t*>(req);

    LOG_F(INFO, "CMD_PUT_MESSAGE");
    LOG_F(INFO, "Message: %s", reinterpret_cast<char*>(put_msg->data));

    res = reinterpret_cast<uds_command_t*>(malloc(sizeof(uds_command_t)));
    if (res != nullptr) {
        res->status = STATUS_SUCCESS;
        res->data_len = 0;
    }

    return reinterpret_cast<uds_command_t*>(res);
}

/*
 * Unknown request type
 */
uds_command_t *cmd_unknown(uds_command_t*) {
    uds_command_t *res;

    LOG_F(INFO, "Unknown request type.");

    res = reinterpret_cast<uds_command_t*>(malloc(sizeof(uds_command_t)));
    if (res != nullptr) {
        res->status = STATUS_INVALID_COMMAND;
        res->data_len = 0;
    }

    return res;
}

/**
 * @brief cmd_operate - Universal operate handler
 * @param req
 * @return
 */
uds_command_t *cmd_operate(uds_command_t *req) {
    LOG_F(INFO, "%s", getCmdAsString(static_cast<uds_command_type>(req->command)));
    if(req->data_len != 0) {
        LOG_F(INFO, "Message: %s", UdsMessage(req).data().c_str());
    }
    return reinterpret_cast<uds_command_t*>(HeadMeters::operate(req));
}

/**
 * @brief request_handler - The handler to handle all requests from client
 * @param req
 * @return
 */
uds_command_t *request_handler(uds_command_t *req) {
    uds_command_t *resp = nullptr;

    switch (req->command) {
    case CMD_GET_VERSION:   resp = cmd_get_version();   break;
    case CMD_GET_MESSAGE:   resp = cmd_get_msg();       break;
    case CMD_PUT_MESSAGE:   resp = cmd_put_msg(req);    break;
    case CMD_START_MEASURE:
    case CMD_SET_RANGE:
    case CMD_STOP_MEASURE:
    case CMD_GET_STATUS:
    case CMD_GET_RESULT:
    case CMD_CHANNELS_INFO: resp = cmd_operate(req);    break;
    default:
        resp = cmd_unknown(req);
        break;
    }

    return resp;
}




