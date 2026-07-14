#include <iostream>
#include "JsonDemo.h"
#include "RecordStore.h"
#include "Cli.h"

int main() {
    runJsonDemo();

    std::cout << "\n";
    RecordStore store("data.json");
    store.load();
    Cli cli(store);
    cli.run();

    return 0;
}
