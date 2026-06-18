#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_PWMServoDriver.h>

// =====================================================
// WIFI
// =====================================================
const char* AP_SSID = "RoboticHand-ESP32";
const char* AP_PASSWORD = "12345678";

WebServer server(80);

// =====================================================
// PCA9685
// =====================================================
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

#define SERVOMIN 150
#define SERVOMAX 600

// =====================================================
// Dedos
// =====================================================
enum Finger
{
  THUMB = 0,
  INDEX = 1,
  MIDDLE = 2,
  RING = 3,
  PINKY = 4
};

bool fingerInverted[5] =
{
  true,   // polegar
  false,  // indicador
  false,  // médio
  false,  // anelar
  false   // mindinho
};

// =====================================================
// Calibrar as posições
// =====================================================
const int OPEN_POS   = 20;
const int REST_POS   = 35;
const int HALF_POS   = 60;
const int CLOSED_POS = 110;
const int WIDE_POS   = 10;

// Polegar — valores independentes
const int THUMB_OPEN_POS   = 20;
const int THUMB_REST_POS   = 35;
const int THUMB_HALF_POS   = 60;
const int THUMB_CLOSED_POS = 110;
const int THUMB_WIDE_POS   = 10;

const int FAST_STEP_DELAY = 4;
const int SLOW_STEP_DELAY = 8;

int currentPos[5] =
{
  THUMB_OPEN_POS, OPEN_POS, OPEN_POS, OPEN_POS, OPEN_POS
};

// =====================================================
// POSE STRUCT
// =====================================================
struct HandPose
{
  int thumb;
  int index;
  int middle;
  int ring;
  int pinky;
};

HandPose makePose(int t, int i, int m, int r, int p)
{
  HandPose h;
  h.thumb = t;
  h.index = i;
  h.middle = m;
  h.ring = r;
  h.pinky = p;
  return h;
}

// =====================================================
// HELPERS
// =====================================================
int clampInt(int v, int mn, int mx)
{
  if (v < mn) return mn;
  if (v > mx) return mx;
  return v;
}

int angleToPulse(int angle)
{
  angle = clampInt(angle, 0, 180);
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

String normalizeCommand(String cmd)
{
  cmd.trim();
  cmd.replace("\r", "");
  cmd.replace("\n", "");
  cmd.toUpperCase();
  cmd.replace(" ", "_");
  cmd.replace("-", "_");
  return cmd;
}

// =====================================================
// Controlador de dedo
// =====================================================
void writeFinger(Finger finger, int logicalAngle)
{
  logicalAngle = clampInt(logicalAngle, 0, 180);
  int realAngle = fingerInverted[finger] ? 180 - logicalAngle : logicalAngle;
  pwm.setPWM((uint8_t)finger, 0, angleToPulse(realAngle));
  currentPos[(int)finger] = logicalAngle;
}

void setPose(const HandPose& h)
{
  writeFinger(THUMB,  h.thumb);
  writeFinger(INDEX,  h.index);
  writeFinger(MIDDLE, h.middle);
  writeFinger(RING,   h.ring);
  writeFinger(PINKY,  h.pinky);
}

void moveToPoseSmooth(const HandPose& target, int stepDelayMs)
{
  int start[5] = { currentPos[0], currentPos[1], currentPos[2], currentPos[3], currentPos[4] };
  int end[5]   = { target.thumb, target.index, target.middle, target.ring, target.pinky };

  int delta[5];
  int maxSteps = 0;

  for (int i = 0; i < 5; i++)
  {
    delta[i] = end[i] - start[i];
    int steps = abs(delta[i]);
    if (steps > maxSteps) maxSteps = steps;
  }

  if (maxSteps == 0) return;

  for (int step = 1; step <= maxSteps; step++)
  {
    for (int i = 0; i < 5; i++)
    {
      int absDelta = abs(delta[i]);
      if (absDelta == 0 || step > absDelta) continue;
      int direction = (delta[i] > 0) ? 1 : -1;
      writeFinger((Finger)i, start[i] + (direction * step));
    }
    delay(stepDelayMs);
  }

  setPose(target);
}

// =====================================================
// POSES
// =====================================================
void openHand()          { moveToPoseSmooth(makePose(THUMB_OPEN_POS,   OPEN_POS,   OPEN_POS,   OPEN_POS,   OPEN_POS),   FAST_STEP_DELAY); }
void closeHand()         { moveToPoseSmooth(makePose(THUMB_CLOSED_POS, CLOSED_POS, CLOSED_POS, CLOSED_POS, CLOSED_POS), FAST_STEP_DELAY); }
void relaxHand()         { moveToPoseSmooth(makePose(THUMB_REST_POS,   REST_POS,   REST_POS,   REST_POS,   REST_POS),   FAST_STEP_DELAY); }
void pointGesture()      { setPose(makePose(THUMB_CLOSED_POS, OPEN_POS,   CLOSED_POS, CLOSED_POS, CLOSED_POS)); }
void holdGesture()       { setPose(makePose(THUMB_HALF_POS,   155,        160,        160,        155)); }
void okGesture()         { setPose(makePose(THUMB_OPEN_POS,   CLOSED_POS, CLOSED_POS, CLOSED_POS, CLOSED_POS)); }

void setFingerFromCode(String code, int value)
{
  value = clampInt(value, 0, 180);
  code = normalizeCommand(code);
  if      (code == "T" || code == "THUMB")  writeFinger(THUMB,  value);
  else if (code == "I" || code == "INDEX")  writeFinger(INDEX,  value);
  else if (code == "M" || code == "MIDDLE") writeFinger(MIDDLE, value);
  else if (code == "R" || code == "RING")   writeFinger(RING,   value);
  else if (code == "P" || code == "PINKY")  writeFinger(PINKY,  value);
}

// =====================================================
// COMMAND PARSER
// =====================================================
void executeCommand(String cmd)
{
  cmd = normalizeCommand(cmd);
  if (cmd.length() == 0) return;

  if      (cmd == "OPEN"  || cmd == "MAO_ABERTA")                  openHand();
  else if (cmd == "CLOSE" || cmd == "PUNHO_FECHADO" || cmd == "PUNHO") closeHand();
  else if (cmd == "POINT" || cmd == "APONTAR")                     pointGesture();
  else if (cmd == "HOLD" || cmd == "SEGURAR")                      holdGesture();
  else if (cmd == "RELAX" || cmd == "RELAXAR" || cmd == "DESCANSO_ECONOMICO") relaxHand();
  else if (cmd == "OK" || cmd == "POLEGAR_PARA_CIMA" || cmd == "THUMB_UP" || cmd == "THUMBS_UP") okGesture();
  else if (cmd.startsWith("T:")) setFingerFromCode("T", String(cmd.substring(2)).toInt());
  else if (cmd.startsWith("I:")) setFingerFromCode("I", String(cmd.substring(2)).toInt());
  else if (cmd.startsWith("M:")) setFingerFromCode("M", String(cmd.substring(2)).toInt());
  else if (cmd.startsWith("R:")) setFingerFromCode("R", String(cmd.substring(2)).toInt());
  else if (cmd.startsWith("P:")) setFingerFromCode("P", String(cmd.substring(2)).toInt());
}

// =====================================================
// WEB PAGE
// =====================================================
const char MAIN_page[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="pt-PT">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
  <meta name="theme-color" content="#0b1b33">
  <title>H.A.N.D: Human Assistant Natural Device</title>
  <style>
    :root{
      --bg:#08111f;
      --bg2:#0d1a2e;
      --bg3:#12213a;
      --panel:rgba(255,255,255,.08);
      --panel2:rgba(255,255,255,.12);
      --line:rgba(255,255,255,.10);
      --line2:rgba(255,255,255,.16);
      --text:#f8fbff;
      --muted:#c2d2e8;
      --muted2:#92abc8;
      --accent:#dff1ff;
      --accent2:#a9d7ff;
      --shadow:0 22px 50px rgba(2,10,26,.42);
      --radius:26px;
      --radius2:30px;
      --max:1280px;
    }

    *{box-sizing:border-box}
    html,body{
      margin:0;
      min-height:100%;
      background:
        radial-gradient(circle at top, rgba(169,215,255,.22), transparent 26%),
        radial-gradient(circle at 20% 18%, rgba(223,241,255,.08), transparent 20%),
        linear-gradient(180deg, #08111f 0%, #0a1528 52%, #07101d 100%);
      color:var(--text);
      font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Inter,Arial,sans-serif;
      -webkit-font-smoothing:antialiased;
      text-rendering:optimizeLegibility;
      scroll-behavior:smooth;
    }

    body{padding:18px 14px 24px}

    .app{
      max-width:var(--max);
      margin:0 auto;
      display:grid;
      gap:16px;
    }

    .topbar,.card,.hero,.footerBar{
      background:linear-gradient(180deg, rgba(16,30,54,.94), rgba(8,17,31,.96));
      border:1px solid var(--line);
      border-radius:var(--radius2);
      box-shadow:var(--shadow);
    }

    .topbar{padding:16px}

    .toprow{
      display:flex;
      justify-content:space-between;
      align-items:flex-start;
      gap:14px;
      flex-wrap:wrap;
    }

    .brand{
      display:flex;
      align-items:center;
      gap:14px;
      min-width:250px;
    }

    .logo{
      width:56px;
      height:56px;
      border-radius:20px;
      display:grid;
      place-items:center;
      font-size:22px;
      font-weight:900;
      letter-spacing:-.04em;
      color:#f7fbff;
      background:linear-gradient(180deg, rgba(223,241,255,.26), rgba(169,215,255,.10));
      border:1px solid rgba(223,241,255,.20);
      box-shadow:inset 0 1px 0 rgba(255,255,255,.08);
    }

    .brand h1{
      margin:0;
      font-size:20px;
      line-height:1.05;
      letter-spacing:-.04em;
      font-weight:900;
    }

    .brand p{
      margin:5px 0 0;
      color:var(--muted);
      font-size:13px;
      line-height:1.45;
    }

    .statusline{
      display:flex;
      flex-wrap:wrap;
      gap:8px;
      align-items:center;
      justify-content:flex-end;
    }

    .pill{
      display:inline-flex;
      align-items:center;
      gap:8px;
      padding:9px 12px;
      border-radius:999px;
      border:1px solid var(--line);
      background:rgba(255,255,255,.05);
      color:var(--muted);
      font-size:12px;
      white-space:nowrap;
    }

    .dot{
      width:10px;
      height:10px;
      border-radius:999px;
      background:#d7e6f7;
      box-shadow:0 0 0 4px rgba(223,241,255,.09);
    }

    .dot.good{background:#eef7ff;box-shadow:0 0 0 4px rgba(223,241,255,.12)}

    .grid2,.grid3{display:grid;gap:16px}
    .grid2{grid-template-columns:1.08fr .92fr}
    .grid3{grid-template-columns:repeat(3,1fr)}

    @media(max-width:980px){.grid2,.grid3{grid-template-columns:1fr}}

    .hero{display:grid;grid-template-columns:1.06fr .94fr;gap:16px;overflow:hidden}
    @media(max-width:980px){.hero{grid-template-columns:1fr}}

    .heroMain{
      min-height:260px;
      padding:22px;
      border-radius:28px;
      background:linear-gradient(180deg, rgba(255,255,255,.06), rgba(255,255,255,.03));
      border:1px solid rgba(255,255,255,.08);
      position:relative;
      overflow:hidden;
      display:flex;
      flex-direction:column;
      justify-content:space-between;
    }

    .heroMain::after{
      content:"";
      position:absolute;
      inset:auto -80px -90px auto;
      width:260px;
      height:260px;
      border-radius:50%;
      background:radial-gradient(circle, rgba(223,241,255,.18), transparent 68%);
      pointer-events:none;
    }

    .eyebrow{
      display:inline-flex;
      align-items:center;
      gap:8px;
      padding:8px 11px;
      border-radius:999px;
      border:1px solid rgba(223,241,255,.16);
      background:rgba(223,241,255,.08);
      color:#f7fbff;
      font-size:12px;
      letter-spacing:.02em;
    }

    .heroTitle{
      margin:12px 0 10px;
      max-width:16ch;
      font-size:34px;
      line-height:1.03;
      letter-spacing:-.05em;
      font-weight:950;
    }

    .heroText{
      margin:0;
      max-width:72ch;
      color:var(--muted);
      font-size:14px;
      line-height:1.72;
    }

    .heroActions{
      display:flex;
      gap:10px;
      flex-wrap:wrap;
      margin-top:18px;
      position:relative;
      z-index:1;
    }

    .sideBox{
      padding:18px;
      border-radius:28px;
      background:linear-gradient(180deg, rgba(11,23,41,.9), rgba(8,17,31,.95));
      border:1px solid var(--line);
      display:flex;
      flex-direction:column;
      gap:12px;
    }

    .welcomeLabel{
      display:inline-flex;
      align-items:center;
      gap:8px;
      padding:8px 11px;
      border-radius:999px;
      border:1px solid rgba(223,241,255,.18);
      background:rgba(223,241,255,.08);
      color:#f7fbff;
      font-size:12px;
      width:fit-content;
    }

    .welcomeTitle{
      margin:0;
      font-size:34px;
      line-height:1.03;
      letter-spacing:-.05em;
      font-weight:950;
    }

    .welcomeSub{
      margin:0;
      max-width:70ch;
      color:var(--muted);
      font-size:14px;
      line-height:1.74;
    }

    .miniGrid{
      display:grid;
      grid-template-columns:1fr 1fr;
      gap:10px;
      margin-top:4px;
    }

    @media(max-width:680px){.miniGrid{grid-template-columns:1fr}}

    .miniCard{
      padding:14px;
      border-radius:18px;
      border:1px solid var(--line);
      background:rgba(255,255,255,.04);
      min-height:92px;
    }

    .miniCard strong{display:block;font-size:14px;margin-bottom:6px}
    .miniCard span{color:var(--muted2);font-size:12px;line-height:1.55}

    .btn{
      border:none;
      border-radius:16px;
      min-height:46px;
      padding:13px 16px;
      color:white;
      font-weight:760;
      font-size:14px;
      cursor:pointer;
      touch-action:manipulation;
      background:linear-gradient(180deg, rgba(223,241,255,.16), rgba(169,215,255,.08));
      border:1px solid rgba(223,241,255,.14);
      box-shadow:inset 0 1px 0 rgba(255,255,255,.05);
      transition:transform .08s ease, filter .14s ease, background .14s ease, border-color .14s ease;
    }

    .btn:hover{filter:brightness(1.06)}
    .btn:active{transform:scale(.985)}
    .btn.primary{background:linear-gradient(180deg, rgba(223,241,255,.22), rgba(169,215,255,.10))}
    .btn.good{background:linear-gradient(180deg, rgba(223,241,255,.20), rgba(169,215,255,.08))}
    .btn.warn{background:linear-gradient(180deg, rgba(223,241,255,.18), rgba(169,215,255,.07))}
    .btn.danger{background:linear-gradient(180deg, rgba(223,241,255,.16), rgba(169,215,255,.06))}
    .btn.soft{background:rgba(255,255,255,.04)}
    .btn.full{width:100%}
    .btn.mini{padding:12px 10px;min-height:50px;font-size:14px}

    .card{overflow:hidden}

    .cardhead{
      padding:16px 16px 12px;
      border-bottom:1px solid rgba(255,255,255,.06);
      display:flex;
      justify-content:space-between;
      align-items:center;
      gap:10px;
    }

    .cardhead h2{margin:0;font-size:16px;font-weight:800;letter-spacing:-.02em}
    .cardhead span{color:var(--muted);font-size:12px;white-space:nowrap}
    .cardbody{padding:16px}

    .gestureGrid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}
    @media(max-width:560px){.gestureGrid{grid-template-columns:1fr}}

    .gestureBtn{
      text-align:left;
      min-height:74px;
      padding:14px;
      display:flex;
      flex-direction:column;
      justify-content:center;
      gap:4px;
      background:linear-gradient(180deg, rgba(255,255,255,.06), rgba(255,255,255,.03));
      border:1px solid var(--line);
      border-radius:18px;
    }

    .gestureBtn strong{font-size:15px;line-height:1.18}
    .gestureBtn small{color:var(--muted2);font-size:12px;line-height:1.35}

    .splitActions{display:grid;grid-template-columns:repeat(2,1fr);gap:10px}
    @media(max-width:560px){.splitActions{grid-template-columns:1fr}}

    .clearRow{display:flex;justify-content:flex-end;margin-bottom:10px}

    .log{
      background:rgba(255,255,255,.04);
      border:1px solid var(--line);
      border-radius:16px;
      padding:12px;
      color:#f7fbff;
      font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;
      font-size:12px;
      line-height:1.55;
      max-height:230px;
      overflow:auto;
      white-space:pre-wrap;
    }

    .fingerWrap{display:grid;gap:12px}

    .fingerItem{
      background:rgba(255,255,255,.04);
      border:1px solid rgba(255,255,255,.07);
      border-radius:18px;
      padding:14px;
    }

    .fingerTop{
      display:flex;
      justify-content:space-between;
      align-items:center;
      gap:10px;
      margin-bottom:10px;
    }

    .fingerTop strong{font-size:15px}
    .fingerTop span{color:var(--muted);font-size:12px}

    .rangeRow{display:grid;grid-template-columns:1fr 56px;gap:10px;align-items:center}
    input[type="range"]{width:100%}

    .val{
      display:grid;
      place-items:center;
      min-height:42px;
      border-radius:12px;
      background:rgba(255,255,255,.05);
      border:1px solid var(--line);
      color:white;
      font-weight:800;
    }

    .footerBar{padding:16px}
    .footerGrid{display:grid;grid-template-columns:repeat(4,1fr);gap:8px}
    @media(max-width:760px){.footerGrid{grid-template-columns:repeat(2,1fr)}}
    @media(max-width:430px){.footerGrid{grid-template-columns:1fr}}
    .footerNote{text-align:center;padding-top:4px;color:var(--muted)}
    .smallNote{margin:0;text-align:center;color:var(--muted2);font-size:12px;line-height:1.55}

    .sectionNote{margin:0 0 12px;color:var(--muted);font-size:12px;line-height:1.6}
    .commandGrid{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}
    @media(max-width:900px){.commandGrid{grid-template-columns:repeat(2,1fr)}}
    @media(max-width:560px){.commandGrid{grid-template-columns:1fr}}

    .commandCard{
      padding:14px;
      border-radius:18px;
      border:1px solid var(--line);
      background:linear-gradient(180deg, rgba(255,255,255,.06), rgba(255,255,255,.03));
      display:flex;
      flex-direction:column;
      gap:10px;
      min-height:138px;
    }

    .commandCard h3{margin:0;font-size:15px;letter-spacing:-.02em}
    .commandCard p{margin:0;color:var(--muted2);font-size:12px;line-height:1.55;flex:1}
    .commandCard .btn{width:100%}

    .stack{display:grid;gap:16px}

    .handCard{
      padding:16px;
      border-radius:28px;
      border:1px solid var(--line);
      background:linear-gradient(180deg, rgba(255,255,255,.06), rgba(255,255,255,.03));
      overflow:hidden;
    }

    .handWrap{
      display:grid;
      grid-template-columns:1.1fr .9fr;
      gap:16px;
      align-items:stretch;
    }

    @media(max-width:980px){
      .handWrap{grid-template-columns:1fr}
    }

    .handPanel{
      border-radius:24px;
      border:1px solid var(--line);
      background:rgba(255,255,255,.04);
      padding:16px;
      min-height:360px;
      display:flex;
      flex-direction:column;
      justify-content:center;
      align-items:center;
      position:relative;
      overflow:hidden;
    }

    .handPanel::before{
      content:"";
      position:absolute;
      inset:0;
      background:
        radial-gradient(circle at 50% 30%, rgba(223,241,255,.16), transparent 24%),
        radial-gradient(circle at 50% 70%, rgba(169,215,255,.08), transparent 28%);
      pointer-events:none;
    }

    .handTitle{
      margin:0 0 10px;
      color:#ffffff;
      font-size:14px;
      font-weight:700;
      letter-spacing:.03em;
      text-transform:uppercase;
      opacity:.92;
      position:relative;
      z-index:1;
    }

    .handCaption{
      margin:10px 0 0;
      text-align:center;
      color:var(--muted);
      font-size:12px;
      line-height:1.55;
      max-width:34ch;
      position:relative;
      z-index:1;
    }

    svg.handSvg{
      width:min(100%, 520px);
      height:auto;
      overflow:visible;
      position:relative;
      z-index:1;
    }

    .palm{fill:rgba(223,241,255,.16);stroke:rgba(223,241,255,.22);stroke-width:2}
    .fingerShape{fill:rgba(223,241,255,.20);stroke:rgba(223,241,255,.25);stroke-width:2}
    .thumbShape{fill:rgba(223,241,255,.18);stroke:rgba(223,241,255,.25);stroke-width:2}
    .joint{fill:rgba(255,255,255,.68)}
    .outline{fill:none;stroke:rgba(255,255,255,.14);stroke-width:2}

    .statusGrid{
      display:grid;
      gap:10px;
      grid-template-columns:repeat(2,1fr);
    }

    @media(max-width:560px){
      .statusGrid{grid-template-columns:1fr}
    }

    .statusBox{
      padding:14px;
      border-radius:18px;
      border:1px solid var(--line);
      background:rgba(255,255,255,.04);
      min-height:100px;
    }

    .statusBox strong{
      display:block;
      margin-bottom:6px;
      font-size:14px;
    }

    .statusBox span{
      color:var(--muted2);
      font-size:12px;
      line-height:1.55;
    }
  </style>
</head>
<body>
  <div class="app">
    <div class="topbar">
      <div class="toprow">
        <div class="brand">
          <div class="logo">H</div>
          <div>
            <h1>H.A.N.D: Human Assistant Natural Device</h1>
            <p>Controlo da mão robótica</p>
          </div>
        </div>
        <div class="statusline">
          <div class="pill"><span class="dot good" id="wifiDot"></span><span id="wifiState">Ligado à rede ESP32</span></div>
          <div class="pill">IP: <strong id="ipState">192.168.4.1</strong></div>
        </div>
      </div>
    </div>

   <div class="hero">
  <div class="heroMain" style="
    grid-column:1/-1;
    max-width:none;
    width:100%;
    min-height:320px;
    display:flex;
    flex-direction:column;
    justify-content:center;
  ">
    <div class="eyebrow">H.A.N.D</div>

    <h2 class="heroTitle">
      Bem vindo Utilizador.
    </h2>

    <p class="heroText" style="max-width:900px;">
      Este projeto foi desenvolvido no âmbito da Prova de Aptidão Profissional (PAP) e consiste na criação de uma mão robótica controlada por um ESP32. O sistema permite controlar individualmente cada dedo, executar gestos predefinidos e interagir através de uma interface web criada para facilitar a utilização do dispositivo. O objetivo principal foi aplicar conhecimentos de programação, eletrónica e automação no desenvolvimento de uma solução funcional e interativa.
    </p>

    <div class="heroActions">
      <button class="btn primary" onclick="applyPreset('OPEN')">Abrir mão</button>
      <button class="btn good" onclick="applyPreset('CLOSE')">Fechar mão</button>
      <button class="btn warn" onclick="applyPreset('POINT')">Apontar</button>
      <button class="btn danger" onclick="applyPreset('HOLD')">Segurar</button>
      <button class="btn" onclick="applyPreset('RELAX')">Relaxar</button>
      <button class="btn" onclick="applyPreset('OK')">Ok</button>
    </div>
  </div>
</div>

    <div class="handCard">
      <div class="handWrap">
        <div class="handPanel">
          <div class="handTitle">Vista da mão</div>
          <svg class="handSvg" viewBox="0 0 760 420" aria-label="Visualização da mão">
            <defs>
              <linearGradient id="gPalm" x1="0" x2="1" y1="0" y2="1">
                <stop offset="0%" stop-color="rgba(223,241,255,.30)"/>
                <stop offset="100%" stop-color="rgba(169,215,255,.12)"/>
              </linearGradient>
            </defs>

            <ellipse cx="380" cy="260" rx="135" ry="110" class="palm" fill="url(#gPalm)"/>
            <path d="M270 250 C300 220, 330 210, 380 210 C430 210, 460 220, 490 250" class="outline"/>

            <g id="thumbGroup" transform="translate(255 235) rotate(-20)">
              <rect x="0" y="0" width="36" height="120" rx="18" class="thumbShape"/>
              <circle cx="18" cy="16" r="7" class="joint"/>
              <circle cx="18" cy="58" r="6" class="joint"/>
              <circle cx="18" cy="100" r="6" class="joint"/>
            </g>

            <g id="indexGroup" transform="translate(320 105) rotate(0)">
              <rect x="0" y="0" width="38" height="150" rx="18" class="fingerShape"/>
              <circle cx="19" cy="16" r="7" class="joint"/>
              <circle cx="19" cy="66" r="6" class="joint"/>
              <circle cx="19" cy="118" r="6" class="joint"/>
            </g>

            <g id="middleGroup" transform="translate(375 90) rotate(0)">
              <rect x="0" y="0" width="40" height="166" rx="18" class="fingerShape"/>
              <circle cx="20" cy="16" r="7" class="joint"/>
              <circle cx="20" cy="74" r="6" class="joint"/>
              <circle cx="20" cy="132" r="6" class="joint"/>
            </g>

            <g id="ringGroup" transform="translate(430 105) rotate(0)">
              <rect x="0" y="0" width="38" height="150" rx="18" class="fingerShape"/>
              <circle cx="19" cy="16" r="7" class="joint"/>
              <circle cx="19" cy="66" r="6" class="joint"/>
              <circle cx="19" cy="118" r="6" class="joint"/>
            </g>

            <g id="pinkyGroup" transform="translate(485 132) rotate(0)">
              <rect x="0" y="0" width="34" height="122" rx="17" class="fingerShape"/>
              <circle cx="17" cy="16" r="7" class="joint"/>
              <circle cx="17" cy="56" r="6" class="joint"/>
              <circle cx="17" cy="96" r="6" class="joint"/>
            </g>
          </svg>
          <p class="handCaption">A visualização acompanha os dedos e muda automaticamente quando mexes nos controlos ou usas um gesto.</p>
        </div>

        <div class="statusGrid">
          <div class="statusBox">
            <strong>Estado geral</strong>
            <span id="poseState">Posição aberta</span>
          </div>
          <div class="statusBox">
            <strong>Ligação</strong>
            <span>Servidor local ativo na rede ESP32.</span>
          </div>
        </div>
      </div>
    </div>

    <div class="grid2">
      <div class="card">
        <div class="cardhead"><h2>Gestos úteis</h2><span>apenas os essenciais</span></div>
        <div class="cardbody">
          <div class="gestureGrid">
            <button class="gestureBtn" onclick="applyPreset('OPEN')"><strong>Abrir Mão</strong><small>posição aberta e neutra</small></button>
            <button class="gestureBtn" onclick="applyPreset('CLOSE')"><strong>Fechar Mão</strong><small>mão fechada para agarrar</small></button>
            <button class="gestureBtn" onclick="applyPreset('POINT')"><strong>Apontar</strong><small>indicar e selecionar</small></button>
            <button class="gestureBtn" onclick="applyPreset('HOLD')"><strong>Segurar</strong><small>preparado para agarrar</small></button>
            <button class="gestureBtn" onclick="applyPreset('RELAX')"><strong>Relaxar</strong><small>reduzir tensão dos servos</small></button>
            <button class="gestureBtn" onclick="applyPreset('OK')"><strong>Ok</strong><small>confirmação rápida</small></button>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="cardhead"><h2>Estado da mão</h2><span>resumo visual</span></div>
        <div class="cardbody">
          <div class="statusGrid">
            <div class="statusBox">
              <strong>Polegar</strong>
              <span id="stateT">20°</span>
            </div>
            <div class="statusBox">
              <strong>Indicador</strong>
              <span id="stateI">20°</span>
            </div>
            <div class="statusBox">
              <strong>Médio</strong>
              <span id="stateM">20°</span>
            </div>
            <div class="statusBox">
              <strong>Anelar</strong>
              <span id="stateR">20°</span>
            </div>
          </div>
          <div style="height:10px"></div>
          <div class="statusBox">
            <strong>Mindinho</strong>
            <span id="stateP">20°</span>
          </div>
        </div>
      </div>
    </div>

    <div class="card">
      <div class="cardhead"><h2>Controlo individual dos dedos</h2><span>movimento automático</span></div>
      <div class="cardbody fingerWrap">
        <div class="fingerItem">
          <div class="fingerTop"><strong>Polegar</strong><span id="Tv">20</span></div>
          <div class="rangeRow">
            <input id="T" type="range" min="0" max="180" value="20" oninput="updateFinger('T', this.value)">
            <div class="val">T</div>
          </div>
        </div>
        <div class="fingerItem">
          <div class="fingerTop"><strong>Indicador</strong><span id="Iv">20</span></div>
          <div class="rangeRow">
            <input id="I" type="range" min="0" max="180" value="20" oninput="updateFinger('I', this.value)">
            <div class="val">I</div>
          </div>
        </div>
        <div class="fingerItem">
          <div class="fingerTop"><strong>Médio</strong><span id="Mv">20</span></div>
          <div class="rangeRow">
            <input id="M" type="range" min="0" max="180" value="20" oninput="updateFinger('M', this.value)">
            <div class="val">M</div>
          </div>
        </div>
        <div class="fingerItem">
          <div class="fingerTop"><strong>Anelar</strong><span id="Rv">20</span></div>
          <div class="rangeRow">
            <input id="R" type="range" min="0" max="180" value="20" oninput="updateFinger('R', this.value)">
            <div class="val">R</div>
          </div>
        </div>
        <div class="fingerItem">
          <div class="fingerTop"><strong>Mindinho</strong><span id="Pv">20</span></div>
          <div class="rangeRow">
            <input id="P" type="range" min="0" max="180" value="20" oninput="updateFinger('P', this.value)">
            <div class="val">P</div>
          </div>
        </div>
      </div>
    </div>

    <div class="footerBar">
      <div class="footerGrid">
        <button class="btn mini primary" onclick="applyPreset('OPEN')">Abrir</button>
        <button class="btn mini good" onclick="applyPreset('CLOSE')">Fechar</button>
        <button class="btn mini warn" onclick="applyPreset('POINT')">Apontar</button>
        <button class="btn mini danger" onclick="applyPreset('HOLD')">Segurar</button>
      </div>
      <div class="footerNote">Prova de Aptidão Profissional José Vassalo</div>
      <p class="smallNote">H.A.N.D: Human Assistant Natural Device</p>
    </div>

    <div class="card">
      <div class="cardhead"><h2>Histórico</h2><span>no final da interface</span></div>
      <div class="cardbody">
        <div class="clearRow"><button class="btn soft" onclick="clearLog()">Limpar log</button></div>
        <div class="log" id="logBox">Pronto.</div>
      </div>
    </div>
  </div>

  <script>
    const logBox = document.getElementById('logBox');
    const sentTimers = {};
    const stateMap = {
      OPEN:  { T: 20, I: 20, M: 20, R: 20, P: 20, name: 'Posição aberta' },
      CLOSE: { T: 110, I: 110, M: 110, R: 110, P: 110, name: 'Mão fechada' },
      POINT: { T: 110, I: 20, M: 110, R: 110, P: 110, name: 'Apontar' },
      HOLD:  { T: 70, I: 155, M: 160, R: 160, P: 155, name: 'Segurar' },
      RELAX: { T: 35, I: 35, M: 35, R: 35, P: 35, name: 'Relaxar' },
      OK:    { T: 20, I: 110, M: 110, R: 110, P: 110, name: 'Ok' }
    };

    function log(msg){
      const time = new Date().toLocaleTimeString('pt-PT', { hour12:false });
      logBox.textContent = `[${time}] ${msg}\n` + logBox.textContent;
    }

    function fetchText(url, okMsg, errMsg){
      return fetch(url, { cache: 'no-store' })
        .then(() => { if (okMsg) log(okMsg); })
        .catch(() => { if (errMsg) log(errMsg); });
    }

    function setSlider(id, value){
      const slider = document.getElementById(id);
      slider.value = value;
      document.getElementById(id + 'v').textContent = value;
      document.getElementById('state' + id).textContent = value + '°';
    }

    function updateFinger(f, value){
      setSlider(f, value);
      updateHandPreview();
      if (sentTimers[f]) clearTimeout(sentTimers[f]);
      sentTimers[f] = setTimeout(() => {
        fetchText('/finger?f=' + encodeURIComponent(f) + '&v=' + encodeURIComponent(value), 'Dedo ' + f + ' = ' + value, 'Erro no dedo ' + f);
      }, 18);
    }

    function mapAngle(value, minRot, maxRot){
      const v = Math.max(0, Math.min(180, Number(value)));
      const t = v / 180;
      return minRot + (maxRot - minRot) * t;
    }

    function updateHandPreview(){
      const T = Number(document.getElementById('T').value);
      const I = Number(document.getElementById('I').value);
      const M = Number(document.getElementById('M').value);
      const R = Number(document.getElementById('R').value);
      const P = Number(document.getElementById('P').value);

      document.getElementById('thumbGroup').setAttribute('transform', 'translate(255 235) rotate(' + mapAngle(T, -28, 42) + ')');
      document.getElementById('indexGroup').setAttribute('transform', 'translate(320 105) rotate(' + mapAngle(I, 0, 78) + ')');
      document.getElementById('middleGroup').setAttribute('transform', 'translate(375 90) rotate(' + mapAngle(M, 0, 78) + ')');
      document.getElementById('ringGroup').setAttribute('transform', 'translate(430 105) rotate(' + mapAngle(R, 0, 78) + ')');
      document.getElementById('pinkyGroup').setAttribute('transform', 'translate(485 132) rotate(' + mapAngle(P, 0, 72) + ')');

      const flex = Math.round((T + I + M + R + P) / 5);
      let label = 'Posição aberta';
      if (flex < 40) label = 'Mão aberta';
      else if (flex < 85) label = 'Posição intermédia';
      else label = 'Mão fechada';
      document.getElementById('poseState').textContent = label;
    }

    function applyPreset(name){
      const preset = stateMap[name];
      if (!preset) return;

      for (const key of ['T','I','M','R','P']) setSlider(key, preset[key]);
      updateHandPreview();

      const commandName = {
        OPEN: 'MAO_ABERTA',
        CLOSE: 'PUNHO_FECHADO',
        POINT: 'APONTAR',
        HOLD: 'SEGURAR',
        RELAX: 'RELAXAR',
        OK: 'OK'
      }[name];

      fetchText('/cmd?name=' + encodeURIComponent(commandName), 'Enviado: ' + preset.name, 'Erro ao enviar: ' + preset.name);
    }

    function clearLog(){
      logBox.textContent = 'Log limpo.';
    }

    document.getElementById('wifiState').textContent = 'Rede ESP32 ativa';
    document.getElementById('wifiDot').className = 'dot good';
    document.getElementById('ipState').textContent = '192.168.4.1';

    updateHandPreview();
  </script>
</body>
</html>
)rawliteral";

// =====================================================
// WEB HANDLERS
// =====================================================
void handleRoot()    { server.send(200, "text/html; charset=utf-8", MAIN_page); }

void handleCmd()
{
  if (!server.hasArg("name")) { server.send(400, "text/plain", "Falta name"); return; }
  executeCommand(server.arg("name"));
  server.send(200, "text/plain", "OK");
}

void handleFinger()
{
  if (!server.hasArg("f") || !server.hasArg("v")) { server.send(400, "text/plain", "Faltam parametros"); return; }
  String f = normalizeCommand(server.arg("f"));
  int v = clampInt(server.arg("v").toInt(), 0, 180);
  setFingerFromCode(f, v);
  server.send(200, "text/plain", "OK");
}

void handleNotFound() { server.send(404, "text/plain", "Nao encontrado"); }

// =====================================================
// SETUP
// =====================================================
void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(50);

  Wire.begin();
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(500);

  openHand();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  server.on("/", handleRoot);
  server.on("/cmd", handleCmd);
  server.on("/finger", handleFinger);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("=================================");
  Serial.println("  H.A.N.D: Human Assistant Natural Device");
  Serial.println("=================================");
  Serial.print("Rede Wi-Fi: "); Serial.println(AP_SSID);
  Serial.print("Password:   "); Serial.println(AP_PASSWORD);
  Serial.print("IP do ESP32: "); Serial.println(WiFi.softAPIP());
}

// =====================================================
// LOOP
// =====================================================
void loop()
{
  server.handleClient();

  if (Serial.available())
  {
    String cmd = Serial.readStringUntil('\n');
    executeCommand(cmd);
  }
}
