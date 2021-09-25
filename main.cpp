#include "service/json_reader/json_reader.h"

#include <iostream>

using namespace std;
using namespace transport_catalogue;

int main() {
    TransportCatalogue transport;

    service::JsonReader json_reader(transport);

    json_reader.ReadQueries(cin, cout);

    return 0;
}
