#include <iostream>

#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

using namespace std;


int main() {

    TransportCatalogue catalogue;

    InputReader filler(catalogue);
    StatReader stats(catalogue);

    filler.ReadQueries();
    stats.ReadQueries();

    return 0;
}
