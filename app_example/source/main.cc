#include "engine_core.h"
#include "example_system.h"

int agnostig_main() {
  auto engine = std::make_unique<lib_core::EngineCore>();
  issue_command(lib_core::AddSystemCommand(
      std::make_unique<app_example::ExampleSystem>(engine.get()), 0));
  return engine->StartEngine();
}

#ifdef WindowsBuild
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
            int nCmdShow) {
  return agnostig_main();
}
#elif UnixBuild
int main(int argc, char** argv) { return agnostig_main(); }
#endif
