#include "control_plane.h"
#include "data_plane.h"
#include <iostream>

int main() {
    control_plane cp;

    cp.add_apn("internet", boost::asio::ip::make_address_v4("10.10.10.1"));
    cp.add_apn("ims", boost::asio::ip::make_address_v4("10.20.30.40"));

    std::cout << '\n';

    auto pdn = cp.create_pdn_connection(
        "internet", 
        boost::asio::ip::make_address_v4("192.168.1.100"),
        12345
    );

    std::cout << '\n';

    auto bearer2 = cp.create_bearer(pdn, 54321);

    std::cout << '\n';

    cp.delete_pdn_connection(pdn->get_cp_teid());

    return 0;
}