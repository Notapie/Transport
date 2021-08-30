#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

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
