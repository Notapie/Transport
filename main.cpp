#include "transport_catalogue.h"
#include "service/input_reader.h"
#include "service/stat_reader.h"

#include <iostream>

using namespace std;


int main() {
    using namespace transport_catalogue;

    TransportCatalogue tc;

    InputReader filler(tc);
    StatReader stats(tc);

    filler.ReadQueries(cin);
    stats.ReadQueries(cin, cout);

    return 0;
}
