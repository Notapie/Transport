#include "service/json_reader/json_reader.h"
#include "transport_catalogue/transport_catalogue.h"

#include <iostream>
#include <fstream>

using namespace std;
using namespace transport_catalogue;

int main() {
    TransportCatalogue transport;

    service::JsonReader json_reader(transport);

    json_reader.ReadJson(cin);
    json_reader.FillCatalogue();

    ofstream out("output.json"s);
    if (!out) {
        cerr << "Could not open output file!"sv;
        return 0;
    }

    json_reader.GetStats(out);

    return 0;
}
