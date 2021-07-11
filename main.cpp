#include <iostream>

#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

using namespace std;


int main() {

    TransportCatalogue transport_catalogue;

    InputReader filler(transport_catalogue);
    StatReader stats(transport_catalogue);

    filler.ReadQueries();
    stats.ReadQueries();

    return 0;
}
