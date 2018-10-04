/*
 * Copyright (c) 2018 Isetta
 */
#include "EngineLoop.h"

#include "Core/Config/Config.h"
#include "Core/Debug/Logger.h"
#include "Core/FileSystem.h"
#include "Core/Memory/MemoryManager.h"
#include "Core/ModuleManager.h"
#include "Core/Time/Clock.h"
#include "Graphics/AnimationNode.h"
#include "Graphics/GUI.h"
#include "Graphics/LightNode.h"
#include "Graphics/ModelNode.h"
#include "Input/Input.h"
#include "Input/InputEnum.h"
#include "Networking/NetworkManager.h"

// TODO(Jacob) Remove, used only for GUIDemo
#include "Core/Color.h"
#include "Core/Math/Rect.h"
#include "Core/Memory/FreeListAllocator.h"
#include "Graphics/GUIStyle.h"
#include "Graphics/RectTransform.h"
#include "imgui/imgui.h"

namespace Isetta {

void RunYidiTest();
void InputDemo();
void NetworkingDemo();
void GraphicsDemo();
void GUIDemo();

void EngineLoop::StartUp() {
  Logger::NewSession();
  Config::Instance().Read("config.cfg");
  if (FileSystem::Instance().FileExists("user.cfg")) {
    Config::Instance().Read("user.cfg");
  }

  intervalTime = 1.0 / Config::Instance().loopConfig.maxFps.GetVal();
  maxSimulationCount = Config::Instance().loopConfig.maxSimCount.GetVal();

  moduleManager.StartUp();

  StartGameClock();
  isGameRunning = true;

  Input::RegisterKeyPressCallback(KeyCode::ESCAPE,
                                  [&]() { isGameRunning = false; });

  // Game Init Part

  NetworkingDemo();
  InputDemo();
  GraphicsDemo();
  GUIDemo();
  RunYidiTest();
}

void EngineLoop::Update() {
  GetGameClock().UpdateTime();

  // TODO(All) Add networking update

  static bool sIsPressed = false;
  static bool cIsPressed = false;

  // Networking update
  if (Input::IsKeyPressed(KeyCode::S)) {
    if (!sIsPressed) {
      sIsPressed = true;
      if (NetworkManager::ServerIsRunning()) {
        for (int i = 0; i < NetworkManager::GetMaxClients(); i++) {
          NetworkManager::SendStringMessageFromServer(i, "Hi!");
        }
      }
    }
  } else {
    sIsPressed = false;
  }
  if (Input::IsKeyPressed(KeyCode::C)) {
    if (!cIsPressed) {
      cIsPressed = true;
      if (NetworkManager::ClientIsConnected()) {
        NetworkManager::SendStringMessageFromClient("Hi!");
      }
    }
  } else {
    cIsPressed = false;
  }

  // end Networking update
  // LOG_INFO(Debug::Channel::General,
  // "//////////////UpdateStart//////////////");

  // Client part
  accumulateTime += GetGameClock().GetDeltaTime();

  for (int i = 0; i < maxSimulationCount && accumulateTime > intervalTime;
       i++) {
    moduleManager.SimulationUpdate(intervalTime);
    // LOG_INFO(Debug::Channel::General,
    // std::to_string(accumulateTime));
    accumulateTime -= intervalTime;
  }

  // TODO(Chaojie) after scenegraph, save previous state for
  // prediction LOG_INFO(Debug::Channel::General,
  // "//////////////Render//////////////");
  moduleManager.RenderUpdate(0);
  // LOG_INFO(Debug::Channel::General,
  // "//////////////UpdateEnd//////////////");
}

void EngineLoop::ShutDown() { moduleManager.ShutDown(); }
void EngineLoop::StartGameClock() const { GetGameClock(); }

void EngineLoop::Run() {
  StartUp();
  while (isGameRunning) {
    Update();
  }
  ShutDown();
}

Clock& EngineLoop::GetGameClock() {
  static Clock gameTime{};
  return gameTime;
}

struct TestObject {
  static const Size size = 123;
  U64 arr[size];

  TestObject(U64 ini) {
    for (Size i = 0; i < size; i++) {
      arr[i] = 10;
    }
  }
  void Print() {
    for (int i = 0; i < size; i++) {
      LOG_INFO(Debug::Channel::General, "%d, %I64u", i, arr[i]);
    }
  }
};

void RunYidiTest() {}

void InputDemo() {
  // TODO(Chaojie) remove later into game logic

  // Register Input callback from player script
  U64 handleA, handleB, handleC;
  handleA = Input::RegisterKeyPressCallback(KeyCode::NUM1, [&handleA]() {
    LOG_INFO(Debug::Channel::General, "A pressed");
    Input::UnregisterKeyPressCallback(KeyCode::A, handleA);
  });
  handleB = Input::RegisterKeyReleaseCallback(KeyCode::A, [&handleB]() {
    LOG_INFO(Debug::Channel::General, "A released");
    Input::UnregisterKeyReleaseCallback(KeyCode::A, handleB);
  });
  handleC = Input::RegisterMousePressCallback(
      MouseButtonCode::MOUSE_LEFT, [&handleC]() {
        LOG_INFO(Debug::Channel::General,
                 {"Left pressed at: " + Input::GetMousePosition().ToString()});
        Input::UnregisterMousePressCallback(MouseButtonCode::MOUSE_LEFT,
                                            handleC);
      });
}
void NetworkingDemo() {
  // Networking
  if (Config::Instance().networkConfig.runServer.GetVal()) {
    NetworkManager::CreateServer(
        Config::Instance().networkConfig.defaultServerIP.GetVal().c_str());
  }
  if (Config::Instance().networkConfig.connectToServer.GetVal()) {
    NetworkManager::ConnectToServer(
        Config::Instance().networkConfig.defaultServerIP.GetVal().c_str(),
        [](bool b) {
          LOG(Debug::Channel::Networking, "Client connection state: %d", b);
        });
  }

  Input::RegisterKeyPressCallback(KeyCode::P, []() {
    if (NetworkManager::ClientIsConnected()) {
      NetworkManager::SendHandleMessageFromClient(0);
    }
  });
  Input::RegisterKeyPressCallback(KeyCode::O, []() {
    if (NetworkManager::ClientIsConnected()) {
      NetworkManager::SendHandleMessageFromClient(1);
    }
  });
  Input::RegisterMousePressCallback(MouseButtonCode::MOUSE_LEFT, []() {
    if (NetworkManager::ClientIsConnected()) {
      NetworkManager::SendHandleMessageFromClient(2);
    }
  });
}
void GraphicsDemo() {
  // TODO(All) Read scene from scene file
  // ModelNode car{"test/Low-Poly-Racing-Car.scene.xml", Math::Vector3{0, -20,
  // 0},
  //              Math::Vector3::zero, Math::Vector3::one};
  static ModelNode car{"push/Pushing.scene.xml", Math::Vector3{-200, -100, 0},
                       Math::Vector3{0, 90, 0}, Math::Vector3::one};
  // ModelNode car{"models/sphere/sphere.scene.xml", Math::Vector3{-200, -100,
  // 0},
  //                     Math::Vector3{0, 90, 0}, Math::Vector3::one};
  static AnimationNode animation{&car};
  animation.AddAnimation("push/Pushing.anim", 0, "", false);
  animation.Play();
  LightNode light{"materials/light.material.xml", Math::Vector3{0, 200, 600},
                  Math::Vector3::zero, Math::Vector3::one};
}
// TODO(Jacob) remove
bool checkbox = false;
void GUIDemo() {
  // GUI Test
  GUI::OnUpdate([&]() {
    GUI::Button(RectTransform{Math::Rect{0, 0, 80, 20}, GUI::Pivot::Bot,
                              GUI::Pivot::Bot},
                "btn", []() { LOG_INFO(Debug::Channel::GUI, "btn"); },
                GUI::ButtonStyle{Color::red, Color::blue, Color::yellow});
    GUI::ButtonImage(RectTransform{{100, 40, 80, 20}}, "btn-id", NULL,
                     []() { LOG_INFO(Debug::Channel::GUI, "btn image"); },
                     GUI::ButtonStyle{Color::grey, Color::green, Color::cyan},
                     GUI::ImageStyle{Color::blue, Color::white, 2}, true);
    GUI::Toggle(RectTransform{{10, 130, 40, 40}}, "toggle me", &checkbox,
                GUI::ButtonStyle{Color::red, Color::blue, Color::yellow});
    GUI::Text(RectTransform{{100, 130, 40, 40}},
              Util::StrFormat("I am %s and I am %d", "Jake", 10),
              GUI::TextStyle{false, false, Color::grey});
    GUI::Label(RectTransform{{100, 230, 40, 40}}, "labelthing", "text",
               GUI::LabelStyle{Color::white, Color::clear});

    static char buffer[1024];
    struct TestCallback {
      static int Callback(GUIInputTextCallbackData* data) {
        LOG_INFO(Debug::Channel::GUI, data->Buf);
        return 0;
      }
    };
    GUI::InputText(RectTransform{{100, 150, 80, 40}}, "inputText", buffer, 1024,
                   GUI::InputStyle{},
                   GUI::InputTextFlags::CallbackCompletion |
                       GUI::InputTextFlags::EnterReturnsTrue,
                   TestCallback::Callback);
    static int val = 0;
    GUI::InputInt(RectTransform{{1000, 150, 100, 40}}, "inputInt", &val);
    static bool open = false;
    open = open || GUI::Button(RectTransform{{100, 700, 80, 20}}, "window btn");
    if (open) {
      GUI::Window(RectTransform{{200, 700, 400, 400}}, "window name",
                  []() {
                    GUI::MenuBar([]() {
                      // ImGui::Text("words in menu");
                      GUI::Menu("menu2", []() {
                        GUI::MenuItem("item2", "Ctrl+99", []() {
                          LOG_INFO(Debug::Channel::GUI, "menu selected");
                        });
                      });
                      GUI::Menu("menu disabled",
                                []() { GUI::MenuItem("item3", "Ctrl+99"); },
                                false);
                    });
                    ImGui::Text("words in window");
                  },
                  &open, GUI::BackgroundStyle{}, GUI::WindowFlags::MenuBar);
    }
    GUI::MenuBar(
        []() {
          GUI::Menu("menu1", []() { GUI::MenuItem("item1", "Ctrl+99"); });
        },
        true);

    GUI::Modal(
        RectTransform{{0, 0, 100, 100}, GUI::Pivot::Center, GUI::Pivot::Center},
        "modal", []() {
          if (ImGui::Button("close", (ImVec2)Math::Vector2(40, 40))) {
            GUI::CloseCurrentPopup();
          }
        });
    if (GUI::Button(RectTransform{{100, 800, 80, 20}}, "modal btn")) {
      GUI::OpenPopup("modal");
    }

    GUI::Draw::Rect(RectTransform{{300, 300, 100, 100}}, Color::blue, 0.1f,
                    GUI::DrawCornerFlags::BotRight, 2.0f);
    GUI::Draw::RectFilled(RectTransform{{300, 300, 10, 150}}, Color::red);

    GUI::Draw::Quad(RectTransform{{0, 0, 0, 0}}, Math::Vector2{600, 30},
                    Math::Vector2{630, 60}, Math::Vector2{630, 90},
                    Math::Vector2{610, 70}, Color::white, 1.5f);
    GUI::Draw::QuadFilled(RectTransform{{0, 0, 0, 0}}, Math::Vector2{600, 130},
                          Math::Vector2{630, 160}, Math::Vector2{630, 190},
                          Math::Vector2{610, 170}, Color::grey);

    GUI::Draw::Triangle(RectTransform{{0, 0, 0, 0}}, Math::Vector2{400, 400},
                        Math::Vector2(440, 440), Math::Vector2(400, 480),
                        Color::green, 3.0f);
    GUI::Draw::TriangleFilled(RectTransform{{0, 0, 0, 0}},
                              Math::Vector2{480, 400}, Math::Vector2{440, 440},
                              Math::Vector2{480, 480}, Color::cyan);

    GUI::Draw::Circle(RectTransform{{530, 530, 0, 0}}, 10, Color::yellow, 6,
                      .1f);

    GUI::Draw::CircleFilled(RectTransform{{530, 80, 0, 0}}, 30, Color::blue,
                            14);
    GUI::Draw::CircleFilled(RectTransform{{530, 80, 0, 0}}, 10, Color::black);

    // GUI::Image(const Math::Rect& position, const TextureID&
    // textureId, const Math::Vector2& size, const Math::Vector2&
    // offset = Math::Vector2::zero, const Math::Vector2& tiling =
    //    Math::Vector2::one,  // TODO(Jacob) what is uv used for?
    // const Color& tint = Color::black,
    // const Color& border = Color::clear);
    static float progress = 0.0f, progressDir = 1.0f;
    if (true) {
      progress += progressDir * 0.4f * GetGameClock().GetDeltaTime();
      if (progress >= +1.1f) {
        progress = +1.1f;
        progressDir *= -1.0f;
      }
      if (progress <= -0.1f) {
        progress = -0.1f;
        progressDir *= -1.0f;
      }
    }
    GUI::ProgressBar(RectTransform{{700, 700, 100, 30}}, progress);
    float progressSaturated =
        (progress < 0.0f) ? 0.0f : (progress > 1.0f) ? 1.0f : progress;
    GUI::ProgressBar(
        RectTransform{{700, 740, 100, 100}}, progress,
        Util::StrFormat("%d/%d", static_cast<int>((312 * progressSaturated)),
                        312));
  });
}
}  // namespace Isetta
