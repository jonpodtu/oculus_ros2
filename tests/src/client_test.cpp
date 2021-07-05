#include <iostream>
#include <sstream>
using namespace std;

#include <narval_oculus/SonarClient.h>
using namespace narval::oculus;

void print_ping(const OculusSimplePingResult& pingMetadata,
                const std::vector<uint8_t>& pingData)
{
    static unsigned int count = 0;
    cout << "=============== Got Ping : " << count++ << endl;
    //cout << pingMetadata << endl;
}

void print_dummy(const OculusMessageHeader& msg)
{
    static unsigned int count = 0;
    cout << "=============== Got dummy : " << count++ << endl;
    //cout << msg << endl;
}

int main()
{
    boost::asio::io_service ioService;
    SonarClient client(ioService);
    
    client.add_ping_callback(&print_ping);
    client.add_dummy_callback(&print_dummy);

    ioService.run(); // is blocking

    return 0;
}


