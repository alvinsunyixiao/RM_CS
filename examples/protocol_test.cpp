#include "serial.h"
#include "protocol.h"
#include <iostream>
#include <unistd.h>
#include <cstring>

using namespace std;

int main() {
#ifdef CPU_ONLY
    CSerial *ser = new CSerial("/dev/tty.usbserial-00000000", 115200);
#else
    CSerial *ser = new CSerial("/dev/ttyTHS2", 115200);
#endif
    Protocol *proto = new Protocol(ser);

    uint8_t fake_tx[MAX_BUFFER_LENGTH] = {0};
    data_u *fake_body = (data_u*)(fake_tx+sizeof(header_t));
    fake_body->aim_request.command_id = AIM_REQUEST;
    fake_body->aim_request.mode = AUTOAIM;

    proto->pack_data(fake_tx, sizeof(aim_request_t));

    proto->run();
    while(1);

    return 0;
}
