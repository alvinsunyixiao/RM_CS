#include "protocol.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

using namespace std;

Protocol::Protocol(CSerial *ser) {
    this->_ser = ser;
    _body_data.idle_msg.command_id = IDLE_MSG;
}

Protocol::~Protocol() {}

char Protocol::get_crc8(void *ptr, uint16_t length, char crc8) {
    char index;
    char *data = (char*)ptr;

    while (length--) {
        index = crc8 ^ *data++;
        crc8 = crc8_table[(uint8_t)index]; 
    }
    return crc8;
}

bool Protocol::check_crc8(void *ptr, uint16_t length) {
    char *data = (char*)ptr;

    if (length <= 2 || !data) {
        cout << "NULL point encoutered or data length too short!" << endl; 
        return false;
    }
    return (get_crc8(ptr, length - 1, k_crc8) == data[length - 1]);
}

void Protocol::append_crc8(void *ptr, uint16_t length) {
    char *data = (char*)ptr;

    if (length <= 2 || !data) {
        cout << "NULL point encoutered or data length too short!" << endl; 
        return;
    }
    data[length - 1] = get_crc8(ptr, length - 1, k_crc8);
}

header_t* Protocol::get_header() {
    if (_ser->bytes_available() < sizeof(header_t))
        return NULL;
    uint16_t recv = _ser->read_bytes(&_header, sizeof(header_t));
#ifdef DEBUG
    char *rxbuf = (char*)&_header;
    if (recv) {
        cout << "head received:" << endl;
        for (size_t i = 0; i < recv; i++)
            printf("%hhu ", rxbuf[i]);
        cout << endl;
    }
    else {
        cout << "no header received" << endl;
    }
#endif
    if (recv != sizeof(header_t) ||
            !check_crc8(&_header, sizeof(header_t))) {
        _ser->flush();
        return NULL;
    }
    
    if (strcmp(_header.irm, IRM)) {
        _ser->flush();
        return NULL;
    }

    return &_header;
}

void Protocol::pack_data(void *ptr, uint16_t length) {
    char *data = (char*)ptr;
    header_t *hd = (header_t*)ptr;
    strcpy(hd->irm, IRM);
    hd->data_length = length;
    append_crc8(data, sizeof(header_t));
    data += sizeof(header_t);
    append_crc8(data, length);
}

data_u* Protocol::get_body() {
    while (_ser->bytes_available() < _header.data_length) {}
    uint16_t recv = _ser->read_bytes(_rxbuf, sizeof(_body_data));
#ifdef DEBUG
    if (recv) {
        cout << "body received:" << endl;
        for (size_t i = 0; i < recv; i++)
            printf("%hhu ", _rxbuf[i]);
        cout << endl;
    }
    else {
        cout << "no body received" << endl;
    }
#endif
    if (recv != _header.data_length ||
            !check_crc8(_rxbuf, _header.data_length)) {
        _ser->flush();
        return NULL;
    }
    
    memcpy(&_body_data, _rxbuf, sizeof(data_u));
    return &_body_data; 
}

void Protocol::process_body() {
    switch (_body_data.aim_request.command_id) {
        case IDLE_MSG:
            break;
        case AIM_REQUEST:
            if (_body_data.aim_request.mode == RUNE) {

            }
            else if (_body_data.aim_request.mode == AUTOAIM) {
                gimbal_control_t *gc = (gimbal_control_t*)(_txbuf + sizeof(header_t));
                gc->command_id = GIMBAL_CONTROL;
                gc->pitch_ref = 9;
                gc->yaw_ref = 10;
                pack_data(_txbuf, sizeof(gimbal_control_t));
            }
            else
                cout << "Body data meaningless!" << endl;
            break;
        default:
            cout << "Message Not Implemented Yet!" << endl;
    }
}

void Protocol::transmit() {
    if (_body_data.aim_request.command_id) {
        _ser->write_bytes(_txbuf, sizeof(header_t) + sizeof(_body_data));
#ifdef DEBUG
        cout << "data transmitted:" << endl;
        for (size_t i = 0; i < sizeof(header_t) + sizeof(_body_data); i++)
            printf("%hhu ", _txbuf[i]);
        printf("\n");
#endif
    }
}
