#include "app.hpp"

int main(int argc, char* argv[]) {
    App app;

    app.init();
    app.run();
    app.quit();
    
    return 0;
}