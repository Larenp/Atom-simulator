/*
 * ATOM SIMULATION  -  CG Mini Project
 * Bohr-model atom viewer: Hydrogen (1) to Krypton (36), up to 4 electron
 * shells.
 *
 * macOS : clang atomsimu.c -Wno-deprecated-declarations -framework GLUT
 * -framework OpenGL -lm -o atomsimu Linux : gcc atomsimu.c -lGL -lGLU -lglut
 * -lm -o atomsimu
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __APPLE__
#include <GLUT/glut.h> /* macOS: Apple's built-in GLUT framework */
#else
#include <GL/glut.h>
#endif

#define PI 3.14159265f
#define MAX_Z 36
#define MAX_SHELLS 4
#define NUM_STARS 110
#define WIN_SIZE 800

/* menu ids (1..MAX_Z are the elements themselves) */
enum { M_START = 101, M_STOP, M_LABELS, M_HOME, M_EXIT };

typedef struct {
  const char *name;
  const char *sym;
  int mass;               /* most common isotope: protons + neutrons */
  int shells[MAX_SHELLS]; /* electrons in K, L, M, N shells          */
} Element;

static const Element elements[MAX_Z] = {
    {"HYDROGEN", "H", 1, {1, 0, 0, 0}},
    {"HELIUM", "He", 4, {2, 0, 0, 0}},
    {"LITHIUM", "Li", 7, {2, 1, 0, 0}},
    {"BERYLLIUM", "Be", 9, {2, 2, 0, 0}},
    {"BORON", "B", 11, {2, 3, 0, 0}},
    {"CARBON", "C", 12, {2, 4, 0, 0}},
    {"NITROGEN", "N", 14, {2, 5, 0, 0}},
    {"OXYGEN", "O", 16, {2, 6, 0, 0}},
    {"FLUORINE", "F", 19, {2, 7, 0, 0}},
    {"NEON", "Ne", 20, {2, 8, 0, 0}},
    {"SODIUM", "Na", 23, {2, 8, 1, 0}},
    {"MAGNESIUM", "Mg", 24, {2, 8, 2, 0}},
    {"ALUMINIUM", "Al", 27, {2, 8, 3, 0}},
    {"SILICON", "Si", 28, {2, 8, 4, 0}},
    {"PHOSPHORUS", "P", 31, {2, 8, 5, 0}},
    {"SULPHUR", "S", 32, {2, 8, 6, 0}},
    {"CHLORINE", "Cl", 35, {2, 8, 7, 0}},
    {"ARGON", "Ar", 40, {2, 8, 8, 0}},
    {"POTASSIUM", "K", 39, {2, 8, 8, 1}},
    {"CALCIUM", "Ca", 40, {2, 8, 8, 2}},
    {"SCANDIUM", "Sc", 45, {2, 8, 9, 2}},
    {"TITANIUM", "Ti", 48, {2, 8, 10, 2}},
    {"VANADIUM", "V", 51, {2, 8, 11, 2}},
    {"CHROMIUM", "Cr", 52, {2, 8, 13, 1}},
    {"MANGANESE", "Mn", 55, {2, 8, 13, 2}},
    {"IRON", "Fe", 56, {2, 8, 14, 2}},
    {"COBALT", "Co", 59, {2, 8, 15, 2}},
    {"NICKEL", "Ni", 58, {2, 8, 16, 2}},
    {"COPPER", "Cu", 63, {2, 8, 18, 1}},
    {"ZINC", "Zn", 64, {2, 8, 18, 2}},
    {"GALLIUM", "Ga", 69, {2, 8, 18, 3}},
    {"GERMANIUM", "Ge", 74, {2, 8, 18, 4}},
    {"ARSENIC", "As", 75, {2, 8, 18, 5}},
    {"SELENIUM", "Se", 80, {2, 8, 18, 6}},
    {"BROMINE", "Br", 79, {2, 8, 18, 7}},
    {"KRYPTON", "Kr", 84, {2, 8, 18, 8}},
};

/* orbit radii (world units) depending on how many shells the atom has */
static const float shellRadius[MAX_SHELLS + 1][MAX_SHELLS] = {
    {0, 0, 0, 0},
    {450, 0, 0, 0},
    {330, 590, 0, 0},
    {290, 490, 690, 0},
    {260, 430, 600, 770}};
static const float shellSpeed[MAX_SHELLS] = {2.2f, 1.5f, 1.0f, 0.7f};
static const float shellDir[MAX_SHELLS] = {1.0f, -1.0f, 1.0f, -1.0f};
static const char *shellName[MAX_SHELLS] = {"K", "L", "M", "N"};

static int value =
    -1; /* -1 = title screen, 0 = pick element, 1..36 = element */
static int animating = 0;
static float speedScale = 1.0f;
static int showLabels = 1;
static unsigned long tick = 0;
static float shellAngle[MAX_SHELLS];
static float viewSize = WIN_SIZE;
static int mainmenu;

static float starX[NUM_STARS], starY[NUM_STARS], starPh[NUM_STARS];

/* ------------------------------------------------------------------ */
/*  small helpers                                                      */
/* ------------------------------------------------------------------ */
static int shellCount(const Element *e) {
  int n = 0;
  for (int i = 0; i < MAX_SHELLS; i++)
    if (e->shells[i] > 0)
      n++;
  return n;
}

static float nucleusRadius(int mass) {
  return 70.0f + 14.0f * sqrtf((float)mass);
}

static void fillDisc(float cx, float cy, float r, int seg) {
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(cx, cy);
  for (int i = 0; i <= seg; i++) {
    float a = 2.0f * PI * i / seg;
    glVertex2f(cx + r * cosf(a), cy + r * sinf(a));
  }
  glEnd();
}

/* soft glow: opaque-ish in the centre, fully transparent at the edge */
static void glowDisc(float cx, float cy, float r, float R, float G, float B,
                     float aCenter) {
  glBegin(GL_TRIANGLE_FAN);
  glColor4f(R, G, B, aCenter);
  glVertex2f(cx, cy);
  glColor4f(R, G, B, 0.0f);
  for (int i = 0; i <= 48; i++) {
    float a = 2.0f * PI * i / 48;
    glVertex2f(cx + r * cosf(a), cy + r * sinf(a));
  }
  glEnd();
}

static void ring(float r) {
  glBegin(GL_LINE_LOOP);
  for (int i = 0; i < 160; i++) {
    float a = 2.0f * PI * i / 160;
    glVertex2f(r * cosf(a), r * sinf(a));
  }
  glEnd();
}

static void frame(float m) {
  glBegin(GL_LINE_LOOP);
  glVertex2f(-m, -m);
  glVertex2f(m, -m);
  glVertex2f(m, m);
  glVertex2f(-m, m);
  glEnd();
}

/* ------------------------------------------------------------------ */
/*  text                                                               */
/* ------------------------------------------------------------------ */
static float textWidth(void *font, const char *s) {
  return glutBitmapLength(font, (const unsigned char *)s) *
         (2000.0f / viewSize);
}

/* colour must be set with glColor before calling */
static void drawText(void *font, float x, float y, const char *s) {
  glRasterPos2f(x, y);
  for (; *s; s++)
    glutBitmapCharacter(font, *s);
}
static void drawCentered(void *font, float y, const char *s) {
  drawText(font, -textWidth(font, s) / 2.0f, y, s);
}
static void drawRight(void *font, float xr, float y, const char *s) {
  drawText(font, xr - textWidth(font, s), y, s);
}
static void drawStrokeCentered(float y, float scale, const char *s) {
  float w = 0;
  for (const char *c = s; *c; c++)
    w += glutStrokeWidth(GLUT_STROKE_ROMAN, *c);
  glPushMatrix();
  glTranslatef(-w * scale / 2.0f, y, 0);
  glScalef(scale, scale, 1);
  for (const char *c = s; *c; c++)
    glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
  glPopMatrix();
}

/* ------------------------------------------------------------------ */
/*  scene pieces                                                       */
/* ------------------------------------------------------------------ */
static void drawBackground(void) {
  glBegin(GL_QUADS);
  glColor3f(0.00f, 0.00f, 0.05f);
  glVertex2f(-1000, -1000);
  glVertex2f(1000, -1000);
  glColor3f(0.02f, 0.06f, 0.22f);
  glVertex2f(1000, 1000);
  glVertex2f(-1000, 1000);
  glEnd();

  glPointSize(2.0f);
  glBegin(GL_POINTS);
  for (int i = 0; i < NUM_STARS; i++) {
    float a = 0.20f + 0.60f * fabsf(sinf(tick * 0.02f + starPh[i]));
    glColor4f(0.8f, 0.85f, 1.0f, a);
    glVertex2f(starX[i], starY[i]);
  }
  glEnd();
}

static void drawNucleus(int z, int a) {
  float rn = nucleusRadius(a);
  float pulse = 0.45f + 0.10f * sinf(tick * 0.05f);

  glowDisc(0, 0, rn * 2.0f, 0.25f, 0.40f, 1.0f, pulse);
  glColor4f(0.05f, 0.10f, 0.65f, 1.0f); /* blue nucleus, as in the original */
  fillDisc(0, 0, rn, 64);

  float nr = 0.78f * rn / sqrtf((float)a);
  if (nr > 0.45f * rn)
    nr = 0.45f * rn;
  for (int i = 0; i < a; i++) {
    float rr = (a == 1) ? 0.0f : (rn - nr) * sqrtf((i + 0.5f) / a);
    float th = i * 2.39996f; /* golden angle packing */
    float x = rr * cosf(th), y = rr * sinf(th);
    int isProton = ((i + 1) * z / a) > (i * z / a);
    if (isProton)
      glColor4f(0.95f, 0.25f, 0.25f, 1.0f);
    else
      glColor4f(0.62f, 0.66f, 0.74f, 1.0f);
    fillDisc(x, y, nr, 24);
    glColor4f(1, 1, 1, 0.35f); /* little highlight => sphere look */
    fillDisc(x - 0.3f * nr, y + 0.3f * nr, 0.35f * nr, 12);
  }
}

static void drawElectron(float x, float y) {
  glowDisc(x, y, 40, 0.35f, 0.65f, 1.0f, 0.65f);
  glColor4f(0.92f, 0.96f, 1.0f, 1.0f);
  fillDisc(x, y, 17, 32);
}

static void drawAtom(int z) {
  const Element *e = &elements[z - 1];
  int ns = shellCount(e);

  for (int s = 0; s < ns; s++) {
    float R = shellRadius[ns][s];
    glLineWidth(5.0f);
    glColor4f(1.0f, 0.25f, 0.25f, 0.12f);
    ring(R);
    glLineWidth(1.6f);
    glColor4f(1.0f, 0.35f, 0.35f, 0.90f);
    ring(R);
  }

  drawNucleus(z, e->mass);

  for (int s = 0; s < ns; s++) {
    int n = e->shells[s];
    float R = shellRadius[ns][s];
    for (int k = 0; k < n; k++) {
      float a = (shellAngle[s] + 360.0f * k / n + 20.0f * s) * PI / 180.0f;
      drawElectron(R * cosf(a), R * sinf(a));
    }
  }
}

static void drawLegend(void) {
  const float x = -900;
  glColor4f(0.95f, 0.25f, 0.25f, 1);
  fillDisc(x, -720, 14, 24);
  glColor4f(0.62f, 0.66f, 0.74f, 1);
  fillDisc(x, -775, 14, 24);
  glowDisc(x, -830, 26, 0.35f, 0.65f, 1.0f, 0.65f);
  glColor4f(0.92f, 0.96f, 1.0f, 1);
  fillDisc(x, -830, 12, 24);
  glLineWidth(1.6f);
  glColor4f(1.0f, 0.35f, 0.35f, 0.9f);
  glBegin(GL_LINES);
  glVertex2f(x - 20, -885);
  glVertex2f(x + 20, -885);
  glEnd();

  glColor4f(1, 1, 1, 1);
  drawText(GLUT_BITMAP_HELVETICA_12, x + 40, -728, "PROTON (+)");
  drawText(GLUT_BITMAP_HELVETICA_12, x + 40, -783, "NEUTRON (0)");
  drawText(GLUT_BITMAP_HELVETICA_12, x + 40, -838, "ELECTRON (-)");
  drawText(GLUT_BITMAP_HELVETICA_12, x + 40, -893, "ORBIT (ENERGY SHELL)");
}

static void drawHelp(void) {
  glColor4f(0.7f, 0.8f, 1.0f, 0.9f);
  drawCentered(GLUT_BITMAP_HELVETICA_10, -965,
               "LEFT / RIGHT: change element   |   SPACE: start / stop   |   + "
               "/ - : speed   |   L: labels   |   B: home   |   Q: quit");
}

/* ------------------------------------------------------------------ */
/*  screens                                                            */
/* ------------------------------------------------------------------ */
static void drawHome(void) {
  glLineWidth(2.0f);
  glColor4f(0.4f, 0.6f, 1.0f, 0.8f);
  frame(960);
  glColor4f(0.4f, 0.6f, 1.0f, 0.35f);
  frame(935);

  glColor4f(1, 1, 1, 1);
  drawCentered(GLUT_BITMAP_HELVETICA_18, 860,
               "Nitte Mahalinga Adyantaya Memorial Institute of Technology");
  glColor4f(0.6f, 0.85f, 1.0f, 1);
  drawCentered(GLUT_BITMAP_HELVETICA_18, 770,
               "DEPARTMENT OF COMPUTER SCIENCE & ENGINEERING");

  glColor4f(1, 1, 1, 1);
  drawCentered(GLUT_BITMAP_HELVETICA_12, 620, "A Mini Project On");

  glLineWidth(3.0f);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  drawStrokeCentered(470, 0.8f, "ATOM SIMULATION");

  /* decorative carbon atom */
  glPushMatrix();
  glTranslatef(0, 110, 0);
  glScalef(0.40f, 0.40f, 1);
  drawAtom(6);
  glPopMatrix();

  glColor4f(0.6f, 0.85f, 1.0f, 1);
  drawCentered(GLUT_BITMAP_HELVETICA_18, -250, "PROJECT BY");
  glColor4f(1, 1, 1, 1);
  drawCentered(GLUT_BITMAP_HELVETICA_18, -330, "Laren Pinto   (NNM23CS258)");
  drawCentered(GLUT_BITMAP_HELVETICA_18, -400, "Anup C   (NNM23CS245)");

  glColor4f(1, 1, 1, 0.45f + 0.55f * (0.5f + 0.5f * sinf(tick * 0.08f)));
  drawCentered(GLUT_BITMAP_HELVETICA_18, -700, "Press ENTER to Continue");
}

static void drawSelect(void) {
  drawNucleus(6, 12);
  glColor4f(1, 1, 1, 1);
  drawCentered(GLUT_BITMAP_HELVETICA_10, -(nucleusRadius(12) + 32), "NUCLEUS");
  drawCentered(GLUT_BITMAP_HELVETICA_18, 895, "SELECT THE ELEMENT USING MENU");
  glColor4f(0.7f, 0.85f, 1.0f, 1);
  drawCentered(
      GLUT_BITMAP_HELVETICA_12, 850,
      "Right-click for the menu   |   or press the LEFT / RIGHT arrow keys");
  drawLegend();
  drawHelp();
}

static void drawElementScreen(int z) {
  const Element *e = &elements[z - 1];
  int ns = shellCount(e);
  char buf[96], cfg[48];

  drawAtom(z);

  if (showLabels) {
    glColor4f(1, 1, 1, 1);
    drawCentered(GLUT_BITMAP_HELVETICA_10, -(nucleusRadius(e->mass) + 32),
                 "NUCLEUS");
    for (int s = 0; s < ns; s++) {
      snprintf(buf, sizeof buf, "%s SHELL: %d", shellName[s], e->shells[s]);
      glColor4f(1.0f, 0.65f, 0.65f, 1);
      drawText(GLUT_BITMAP_HELVETICA_10, 14, shellRadius[ns][s] + 12, buf);
    }
  }

  snprintf(buf, sizeof buf, "%s  (%s)", e->name, e->sym);
  glColor4f(1, 1, 1, 1);
  drawCentered(GLUT_BITMAP_HELVETICA_18, 895, buf);
  snprintf(buf, sizeof buf, "ATOMIC NUMBER %d   |   MASS NUMBER %d", z,
           e->mass);
  glColor4f(0.7f, 0.85f, 1.0f, 1);
  drawCentered(GLUT_BITMAP_HELVETICA_12, 850, buf);

  /* info panel (bottom right) */
  int off = 0;
  for (int s = 0; s < ns; s++) {
    if (s)
      off += snprintf(cfg + off, sizeof cfg - off, ", ");
    off += snprintf(cfg + off, sizeof cfg - off, "%d", e->shells[s]);
  }
  glColor4f(1, 1, 1, 1);
  snprintf(buf, sizeof buf, "PROTONS : %d", z);
  drawRight(GLUT_BITMAP_HELVETICA_12, 900, -690, buf);
  snprintf(buf, sizeof buf, "NEUTRONS : %d", e->mass - z);
  drawRight(GLUT_BITMAP_HELVETICA_12, 900, -740, buf);
  snprintf(buf, sizeof buf, "ELECTRONS : %d", z);
  drawRight(GLUT_BITMAP_HELVETICA_12, 900, -790, buf);
  snprintf(buf, sizeof buf, "ELECTRONS PER SHELL : %s", cfg);
  drawRight(GLUT_BITMAP_HELVETICA_12, 900, -840, buf);
  snprintf(buf, sizeof buf, "OUTER SHELL ELECTRONS : %d", e->shells[ns - 1]);
  drawRight(GLUT_BITMAP_HELVETICA_12, 900, -890, buf);

  drawLegend();
  drawHelp();
}

/* ------------------------------------------------------------------ */
/*  GLUT callbacks                                                     */
/* ------------------------------------------------------------------ */
static void display(void) {
  glClearColor(0, 0, 0.1f, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  glLoadIdentity();

  drawBackground();
  if (value == -1)
    drawHome();
  else if (value == 0)
    drawSelect();
  else
    drawElementScreen(value);

  glutSwapBuffers();
}

static void reshape(int w, int h) {
  int s = (w < h) ? w : h; /* keep the 1:1 aspect so circles stay round */
  viewSize = (float)s;
  glViewport((w - s) / 2, (h - s) / 2, s, s);
}

static void timer(int v) {
  tick++;
  if ((animating && value >= 1) || value == -1) {
    for (int s = 0; s < MAX_SHELLS; s++) {
      shellAngle[s] += shellDir[s] * shellSpeed[s] * speedScale;
      if (shellAngle[s] >= 360.0f)
        shellAngle[s] -= 360.0f;
      if (shellAngle[s] < 0.0f)
        shellAngle[s] += 360.0f;
    }
  }
  glutPostRedisplay();
  glutTimerFunc(16, timer, 0);
}

static void setElement(int z) {
  value = z;
  animating = 1;
}

static void mouseControl(int button, int state, int x, int y) {
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    animating = 1;
}

static void keyboard(unsigned char key, int x, int y) {
  if (key == 13) {
    if (value == -1)
      value = 0;
  } else if (key == 's' || key == 'S') {
    animating = 0;
  } else if (key == 32) {
    animating = !animating;
  } else if (key == '+' || key == '=') {
    speedScale *= 1.25f;
    if (speedScale > 6.0f)
      speedScale = 6.0f;
  } else if (key == '-' || key == '_') {
    speedScale /= 1.25f;
    if (speedScale < 0.2f)
      speedScale = 0.2f;
  } else if (key == 'l' || key == 'L') {
    showLabels = !showLabels;
  } else if (key == 'b' || key == 'B') {
    value = -1;
  } else if (key == 'q' || key == 'Q') {
    exit(0);
  } else if (key == 27) {
    glutReshapeWindow(WIN_SIZE, WIN_SIZE);
  }
}

static void fkey(int key, int x, int y) {
  if (key == GLUT_KEY_F10) {
    glutReshapeWindow(glutGet(GLUT_SCREEN_WIDTH), glutGet(GLUT_SCREEN_HEIGHT));
  } else if (key == GLUT_KEY_RIGHT && value >= 0) {
    setElement(value >= MAX_Z ? 1 : value + 1);
  } else if (key == GLUT_KEY_LEFT && value >= 0) {
    setElement(value <= 1 ? MAX_Z : value - 1);
  }
}

static void menu(int option) {
  if (option >= 1 && option <= MAX_Z) {
    setElement(option);
  } else {
    switch (option) {
    case M_START:
      animating = 1;
      break;
    case M_STOP:
      animating = 0;
      break;
    case M_LABELS:
      showLabels = !showLabels;
      break;
    case M_HOME:
      value = -1;
      break;
    case M_EXIT:
      exit(0);
    }
  }
  glutPostRedisplay();
}

static void createMenu(void) {
  int sub[4];
  char label[64];
  const int range[4][2] = {{1, 2}, {3, 10}, {11, 18}, {19, 36}};
  const char *title[4] = {"PERIOD 1  (H - He)", "PERIOD 2  (Li - Ne)",
                          "PERIOD 3  (Na - Ar)", "PERIOD 4  (K - Kr)"};

  for (int p = 0; p < 4; p++) {
    sub[p] = glutCreateMenu(menu);
    for (int z = range[p][0]; z <= range[p][1]; z++) {
      snprintf(label, sizeof label, "%2d  %s (%s)", z, elements[z - 1].name,
               elements[z - 1].sym);
      glutAddMenuEntry(label, z);
    }
  }
  mainmenu = glutCreateMenu(menu);
  for (int p = 0; p < 4; p++)
    glutAddSubMenu(title[p], sub[p]);
  glutAddMenuEntry("START SIMULATION", M_START);
  glutAddMenuEntry("STOP SIMULATION", M_STOP);
  glutAddMenuEntry("TOGGLE LABELS", M_LABELS);
  glutAddMenuEntry("GOTO HOME SCREEN", M_HOME);
  glutAddMenuEntry("EXIT", M_EXIT);
  glutSetMenu(mainmenu);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}

static void init(void) {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-1000, 1000, -1000, 1000);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_POINT_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
#ifdef GL_MULTISAMPLE
  glEnable(GL_MULTISAMPLE);
#endif

  unsigned int s = 12345u;
  for (int i = 0; i < NUM_STARS; i++) {
    s = s * 1664525u + 1013904223u;
    starX[i] = ((s >> 8) & 0xFFFF) / 65535.0f * 1900.0f - 950.0f;
    s = s * 1664525u + 1013904223u;
    starY[i] = ((s >> 8) & 0xFFFF) / 65535.0f * 1900.0f - 950.0f;
    s = s * 1664525u + 1013904223u;
    starPh[i] = ((s >> 8) & 0xFFFF) / 65535.0f * 2.0f * PI;
  }
}

int main(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitWindowPosition(100, 100);
  glutInitWindowSize(WIN_SIZE, WIN_SIZE);
#ifdef __APPLE__
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_MULTISAMPLE);
#else
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
#endif
  glutCreateWindow("ATOM SIMULATION");
  init();
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouseControl);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(fkey);
  createMenu();
  glutTimerFunc(16, timer, 0);
  glutMainLoop();
  return 0;
}