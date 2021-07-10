#include <iostream>

#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

using namespace std;


int main() {

    TransportCatalogue catalogue;

    InputReader filler(catalogue);
    StatReader stats(catalogue);

    filler.ReadQueries();
    stats.ReadQueries();

    return 0;
}
