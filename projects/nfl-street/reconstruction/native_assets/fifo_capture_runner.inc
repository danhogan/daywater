// Research-only addition to a private copy of the ModernGekko host runner.
// Links its existing Dolphin libraries; never part of the native game renderer.
#include <atomic>
static std::jthread StartResearchFifoCapture(moderngekko::Runtime& runtime) {
  const char* output = std::getenv("DAYWATER_FIFO_CAPTURE");
  if (!output || !*output) return {};
  const std::string prefix(output);
  return std::jthread([&runtime, prefix](std::stop_token stop) {
    auto& system = Core::System::GetInstance();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
    auto expired = [&] { return stop.stop_requested() || std::chrono::steady_clock::now() >= deadline; };
    while (!expired() && Core::GetState(system) != Core::State::Running)
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (expired()) { runtime.RequestStop(); return; }
    // Allow boot/session restoration, then reload from a paused CPU to anchor
    // the run to the exact supplied saved state rather than elapsed wall time.
    std::this_thread::sleep_for(std::chrono::seconds(2));
    if (runtime.Pause()) { runtime.RequestStop(); return; }
    if (const auto& saved = runtime.GetConfig().load_state_path; saved)
      State::LoadAs(system, saved->string());
    else { std::cerr << "FIFO capture requires --load-state\n"; runtime.RequestStop(); return; }
    auto& recorder = system.GetFifoRecorder();
    auto complete = std::make_shared<std::atomic_bool>(false);
    recorder.StartRecording(2, [complete] { complete->store(true); });
    // Run continuously for the recorder's bounded two-frame interval. Frame
    // stepping in this saved-state host can leave the VI boundary pending.
    if (runtime.Resume()) { recorder.StopRecording(); runtime.RequestStop(); return; }
    unsigned steps = 0;
    while (!expired() && !complete->load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    runtime.Pause();
    auto* file = recorder.GetRecordedFile();
    bool saved = !expired() && complete->load() && file && file->GetFrameCount() == 2 && file->Save(prefix + ".dff");
    std::ofstream metadata(prefix + ".txt");
    metadata << "saved=" << saved << "\nframes=" << (file ? file->GetFrameCount() : 0)
             << "\nframe_step_requests=" << steps << "\nscreenshot_exact_recorded_frame=0\n";
    metadata.close();
    if (saved) {
      { const Core::CPUThreadGuard guard(system);
        if (g_frame_dumper) g_frame_dumper->SaveScreenshot(prefix + ".png"); }
      Core::DoFrameStep(system);
      const auto image_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      while (!stop.stop_requested() && std::chrono::steady_clock::now() < image_deadline && !std::filesystem::exists(prefix + ".png"))
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    recorder.StopRecording();
    std::cout << "Research FIFO capture: " << saved << ", steps " << steps << '\n';
    runtime.RequestStop();
  });
}
