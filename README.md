# 2D simulacija mašine za parfeme

2D simulacija vending mašine za parfeme, izrađena u C++ i OpenGL-u 3.3 (core profile). Projekat iz predmeta Računarska grafika.

Ovaj fajl opisuje kako je projekat napravljen: strukturu, kako delovi rade zajedno i zašto su implementirani baš tako. Teorija na kojoj se projekat zasniva (OpenGL kao specifikacija, mašina stanja, objekti i slotovi, grafički pajplajn, VBO/VAO/EBO, šejderi, uniforme, blending, teksture) nalazi se u [learnopengl.md](docs/learnopengl.md).

## Podešavanje i pokretanje

Projekat je prazan C++ projekat (Empty Project) u Visual Studiju, platforma x64.

Biblioteke su dodate preko NuGet-a:
- **glfw** 3.4.0,
- **glew-2.2.0** 2.2.0.1.

NuGet paketi se smeštaju u folder `packages/` pored projekta. U `.gitignore` su zakomentarisana pravila koja ignorišu taj folder, pa su biblioteke deo repozitorijuma i projekat se na drugom računaru otvara i pokreće bez interneta i bez dodatnog podešavanja.

U Project → Properties → Linker → Input → Additional Dependencies dodat je `opengl32.lib`, Windowsova OpenGL biblioteka.

Program se pokreće iz Visual Studija (F5). Tada je radni folder folder projekta, pa se šejderi (`default.vert`, `default.frag`) i slike (`res/`) učitavaju relativnim putanjama.

## Struktura projekta

```
PerfumeVendingMachine/
├── Main.cpp              prozor, petlja, raspored mašine, crtanje
├── shaderClass.h/.cpp    Shader: čita, kompajlira i povezuje šejdere, postavlja uniforme
├── VBO.h/.cpp            bafer sa verteksima
├── EBO.h/.cpp            bafer sa indeksima
├── VAO.h/.cpp            opis kako se čitaju verteksi
├── Texture.h/.cpp        učitavanje slike i pravljenje teksture
├── Util.h/.cpp           kod biblioteke stb_image i pravljenje kursora
├── stb_image.h           biblioteka za učitavanje slika
├── default.vert          vertex šejder
├── default.frag          fragment šejder
└── res/                  sve slike (PNG)
```

Struktura sa posebnom klasom za svaki OpenGL objekat preuzeta je iz tutorijala Victora Gordana. Svaka klasa je tanak omotač oko nekoliko OpenGL funkcija i čuva samo id objekta (`GLuint ID`), pa je `Main.cpp` kraći i čitljiviji. Ono što ne pripada nijednoj klasi (stb_image i kursor) je u `Util.cpp`, kao na vežbama.

## Tok programa

```
1.  glfwInit, glfwWindowHint ×3     GLFW i OpenGL 3.3 core
2.  glfwCreateWindow                prozor preko celog ekrana
3.  glfwMakeContextCurrent          kontekst
4.  glfwSwapInterval(0)             v-sync isključen
5.  glfwSetKeyCallback              Escape
6.  loadImageToCursor, glfwSetCursor  kursor bočica
7.  glewInit                        OpenGL funkcije
8.  glViewport, updateLayout        veličina ekrana i raspored mašine
9.  Shader                          šejderi
10. VAO, VBO, EBO                   jedan kvadrat
11. Texture ×N                      slike
12. uniforme, blending              jednokratna podešavanja

13. PETLJA (75 puta u sekundi):
      glClear                       prefarbaj ekran
      crtanje                       pozadina, sjenka, mašina, natpis...
      glfwSwapBuffers               prikaži sliku
      glfwPollEvents                tastatura → callback
      limiter                       čekaj do 1/75 s

14. Delete, glfwDestroyCursor       brisanje resursa
15. glfwDestroyWindow, glfwTerminate
```

Redosled koraka 1 do 12 nije proizvoljan: nema konteksta bez prozora, nema GLEW-a bez konteksta, nema šejdera, bafera ni tekstura bez GLEW-a.

## Prozor, Escape i 75 FPS

```cpp
GLFWmonitor* monitor = glfwGetPrimaryMonitor();
const GLFWvidmode* mode = glfwGetVideoMode(monitor);
GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Perfume Vending Machine", monitor, NULL);
```

**Full screen:** prozor se pravi u rezoluciji glavnog monitora, a četvrti parametar (`monitor`) ga čini full screen-om. Prozor nikad ne menja veličinu, pa callback za promenu veličine nije potreban.

```cpp
glfwGetFramebufferSize(window, &screenW, &screenH);
glViewport(0, 0, screenW, screenH);
```

**Veličina ekrana** se uzima iz framebuffera, a ne iz monitora, jer se na računarima sa Windows skaliranjem (125%, 150%) one mogu razlikovati. Sve pozicije u programu računaju se iz `screenW` i `screenH`, pa projekat radi na bilo kojoj rezoluciji.

**Escape** se obrađuje callback-om (`keyboardCallback`), kao na vežbama. Callback samo postavlja zastavicu, pa se program uvek gasi kroz čišćenje resursa na kraju.

```cpp
glfwSwapInterval(0);
...
while (glfwGetTime() - frameStart < TARGET_FRAME_TIME) {}
```

**75 FPS:** v-sync je isključen, jer bi inače brzinu određivao monitor. Na kraju svakog frame-a petlja čeka dok od početka frame-a ne prođe 1/75 sekunde (13,3 ms). To je aktivno čekanje (busy wait): troši procesor, ali je precizno i jednostavno.

## Jedan kvadrat za sve

U celom projektu postoji **samo jedan oblik**: kvadrat 1x1 sa centrom u (0, 0).

```cpp
GLfloat vertices[] = {
    -0.5f, -0.5f,   0.0f, 0.0f,   // donji levi
     0.5f, -0.5f,   1.0f, 0.0f,   // donji desni
     0.5f,  0.5f,   1.0f, 1.0f,   // gornji desni
    -0.5f,  0.5f,   0.0f, 1.0f,   // gornji levi
};
GLuint indices[] = {
    0, 1, 2,
    0, 2, 3,
};
```

Svaki verteks ima poziciju (x, y) i koordinatu teksture (u, v). Projekat je 2D, pa se z ne šalje.

**Zašto kvadrat:** svaka slika u projektu (mašina, bočica, kovanica, dugme) je pravougaona. Oblik ne dolazi iz geometrije, nego iz providnosti: oko kovanice su pikseli PNG-a potpuno providni, pa se vidi samo krug. Zato je dovoljan jedan VBO, jedan EBO i jedan VAO, a svi elementi se razlikuju samo po uniformama i teksturi.

**Zašto od -0.5 do 0.5:** centar kvadrata je u (0, 0), pa se objekat zadaje svojim centrom. Kad se kvadrat uveća, raste ravnomerno na sve strane i ostaje na istom mestu, što je zgodno za kursor, kovanicu koja leti i animacije.

```cpp
VAO VAO1;
VAO1.Bind();
VBO VBO1(vertices, sizeof(vertices));
EBO EBO1(indices, sizeof(indices));
VAO1.LinkAttrib(VBO1, 0, 2, GL_FLOAT, 4 * sizeof(float), (void*)0);
VAO1.LinkAttrib(VBO1, 1, 2, GL_FLOAT, 4 * sizeof(float), (void*)(2 * sizeof(float)));
VAO1.Unbind();
VBO1.Unbind();
EBO1.Unbind();
```

Ovo se radi jednom, pre petlje. VAO se veže prvi, EBO se pravi dok je VAO vezan, a VAO se razvezuje pre EBO-a (zašto, objašnjeno je u TEORIJA.md, u delu o VAO-u). `GL_STATIC_DRAW` jer se kvadrat nikad ne menja.

Crta se sa `glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0)`: dva trougla iz 6 indeksa. Na vežbama je isti kvadrat crtan sa `glDrawArrays(GL_TRIANGLE_FAN, 0, 4)`, bez EBO-a. Rezultat je isti.

## Šejderi u projektu

Projekat ima jedan šejder program (`default.vert` + `default.frag`), kojim se crta sve.

### Vertex šejder: pikseli umesto NDC

```glsl
vec2 pixel = uPos + aPos * uSize;
vec2 ndc = pixel / uScreen * 2.0 - 1.0;
gl_Position = vec4(ndc, 0.0, 1.0);

texCoord = uUVRect.xy + aTex * uUVRect.zw;
localPos = aPos;
```

U `Main.cpp` se sve zadaje **u pikselima ekrana**, jer je lakše reći "sredina ovde, širok 270 piksela" nego računati u opsegu od -1 do 1, posebno što ekran nije kvadratan. Šejder pretvara piksele u NDC:

1. `uPos + aPos * uSize`: kvadrat 1x1 se razvuče na `uSize` i pomeri tako da mu centar bude u `uPos`.
2. `pixel / uScreen * 2.0 - 1.0`: pikseli se prebace u opseg od -1 do 1.

Primer za kovanicu sa centrom u (768, 540), veličine 270x270, na ekranu 1920x1080:

| Ugao | `aPos` | `pixel` | `ndc` |
|---|---|---|---|
| donji levi | (-0.5, -0.5) | (633, 405) | (-0.34, -0.25) |
| donji desni | (0.5, -0.5) | (903, 405) | (-0.06, -0.25) |
| gornji desni | (0.5, 0.5) | (903, 675) | (-0.06, 0.25) |
| gornji levi | (-0.5, 0.5) | (633, 675) | (-0.34, 0.25) |

`texCoord` bira koji deo slike ide na kvadrat (preko `uUVRect`), a `localPos` je pozicija unutar kvadrata, koju fragment šejder koristi za krug.

### Fragment šejder: jedan šejder, više načina crtanja

```glsl
vec4 color;
if (uUseTexture == 1)
    color = texture(tex0, texCoord) * vec4(uTint, 1.0);
else
    color = uColor;

float d = length(localPos) * 2.0;
if (uShape == 1)
    color.a *= 1.0 - smoothstep(0.92, 1.0, d);
else if (uShape == 2)
    color.a *= 1.0 - smoothstep(0.0, 1.0, d);

color.a *= uAlpha;
if (color.a < 0.003)
    discard;

FragColor = color;
```

Umesto više šejdera, jedan šejder ima "prekidače" u obliku uniformi. Time se izbegava menjanje šejder programa između crtanja.

### Uniforme

| Uniforma | Gde | Šta radi | Kada se postavlja |
|---|---|---|---|
| `uScreen` | vertex | veličina ekrana u pikselima | jednom, pre petlje |
| `uPos` | vertex | centar objekta u pikselima | pre svakog crtanja |
| `uSize` | vertex | širina i visina objekta u pikselima | pre svakog crtanja |
| `uUVRect` | vertex | koji deo slike: početak (`xy`) i veličina (`zw`); (0, 0, 1, 1) je cela slika | pre svakog crtanja |
| `tex0` | fragment | teksturna jedinica (uvek 0) | jednom, pre petlje |
| `uUseTexture` | fragment | 1 = boja iz slike, 0 = boja iz `uColor` | pre svakog crtanja |
| `uColor` | fragment | jednobojna boja (RGBA) | kad se crta bojom |
| `uTint` | fragment | množi boju slike; (1, 1, 1) ne menja ništa, manje vrednosti zatamne, a npr. (1, 0.85, 0.4) boji belo u žuto | pre svakog crtanja |
| `uShape` | fragment | 0 = pravougaonik, 1 = krug, 2 = mekan sjaj | pre svakog crtanja |
| `uAlpha` | fragment | providnost celog objekta | pre svakog crtanja |

`uShape = 1` pravi krug tako što su pikseli dalje od poluprečnika providni, a tanak pojas na ivici blago providan (`smoothstep`), pa ivica nije nazubljena. `uShape = 2` pravi sjaj koji bledi od centra ka ivici. Za slike je uvek 0, jer oblik daje sama slika.

## Klase

### Shader

Konstruktor čita `default.vert` i `default.frag`, kompajlira ih, povezuje u program i briše pojedinačne šejdere. `compileErrors` ispisuje grešku drajvera ako kompajliranje ili linkovanje ne uspe, pa greška u šejderu ne ostaje samo prazan ekran. `get_file_contents` ispisuje i koji fajl nije pronađen.

`setInt`, `setFloat`, `setVec2`, `setVec3` i `setVec4` su isto što i `glUniform...(glGetUniformLocation(...), ...)` sa vežbi, samo umotano u metode. Pošto `glUniform` upisuje u aktivan program, pre postavljanja uniformi uvek ide `shaderProgram.Activate()`.

### VBO, EBO, VAO

Konstruktori prave i pune bafere, `LinkAttrib` opisuje jedan atribut, a `Bind`, `Unbind` i `Delete` su omotači oko `glBind...` i `glDelete...`.

### Texture

```cpp
stbi_set_flip_vertically_on_load(true);
unsigned char* bytes = stbi_load(image, &width, &height, &numColCh, 4);
```

Odluke u klasi `Texture`:
- **uvek 4 kanala (RGBA):** sve slike u projektu imaju providnost, pa je format uvek isti i ne mora da se proverava broj kanala, kao u `loadImageToTexture` na vežbama,
- **slika se okreće** pri učitavanju, jer OpenGL čita teksturu odozdo,
- **`GL_CLAMP_TO_EDGE`** umesto `GL_REPEAT`: slike se nikad ne ponavljaju, a sa `GL_REPEAT` bi linearno filtriranje na samoj ivici moglo da pomeša piksele sa suprotne strane slike. Bitno je za atlas cifara, gde je svaka cifra isečak iste slike,
- **`GL_LINEAR_MIPMAP_LINEAR` + mipmape** za umanjivanje i `GL_LINEAR` za uvećavanje, pa slike izgledaju glatko na svakoj rezoluciji,
- **čuva `width` i `height`** slike, da bi se natpisi mogli crtati u pravim proporcijama,
- **ako slika ne postoji**, ispiše se greška i napravi se tekstura od jednog ljubičastog piksela, pa program ne pukne, a odmah se vidi šta fali.

Sve teksture koriste teksturnu jedinicu 0, jer se u jednom crtanju koristi samo jedna slika. `tex0` se jednom poveže sa jedinicom 0:

```cpp
texBackground.texUnit(shaderProgram, "tex0", 0);
```

### Brisanje bez destruktora

Klase nemaju destruktor koji poziva `glDelete...`. Objekti iz `main` bi bili uništeni tek posle `glfwTerminate`, kad konteksta više nema, a OpenGL poziv tada može da sruši program. Zato se brisanje radi ručno, metodom `Delete()`, pre gašenja GLFW-a.

## Util

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
```

`Util.cpp` je jedini fajl sa `#define STB_IMAGE_IMPLEMENTATION`, pa se kod biblioteke stb_image kompajlira tačno jednom. `Texture.cpp` i `Main.cpp` uključuju `stb_image.h` bez toga. Na vežbama je isto bilo u `Util.cpp`.

```cpp
GLFWcursor* loadImageToCursor(const char* filePath, int hotspotX, int hotspotY)
{
    stbi_set_flip_vertically_on_load(false);

    int width, height, channels;
    unsigned char* imageData = stbi_load(filePath, &width, &height, &channels, 4);
    if (imageData == NULL)
    {
        std::cout << "Failed to load cursor: " << filePath << std::endl;
        return nullptr;
    }

    GLFWimage image;
    image.width = width;
    image.height = height;
    image.pixels = imageData;

    GLFWcursor* cursor = glfwCreateCursor(&image, hotspotX, hotspotY);
    stbi_image_free(imageData);
    return cursor;
}
```

Isto kao funkcija sa vežbi, uz tri razlike:
- **`stbi_set_flip_vertically_on_load(false)`:** GLFW čita sliku odozgo, kao PNG, pa se kursor ne okreće. Pošto `Texture` uključuje okretanje, a to podešavanje važi za celu biblioteku, ovde se mora eksplicitno isključiti.
- **4 kanala umesto 0:** `glfwCreateCursor` zahteva RGBA.
- **`return nullptr`** ako slika ne postoji (verzija sa vežbi tu nema `return`).

`hotspotX` i `hotspotY` su tačka klika na slici kursora, u pikselima slike.

## Crtanje: `Rect`, `drawTexture`, `drawColor`

```cpp
struct Rect {
    float cx, cy, w, h;
    bool contains(float x, float y) const { ... }
};
```

`Rect` opisuje pravougaonik na ekranu u pikselima: `cx` i `cy` su koordinate **centra** (koliko je centar udaljen od leve, odnosno donje ivice ekrana), a `w` i `h` širina i visina. `contains` proverava da li je tačka (npr. klik miša) unutar pravougaonika.

```cpp
void drawTexture(Texture& tex, const Rect& r, float alpha = 1.0f,
                 float tr = 1.0f, float tg = 1.0f, float tb = 1.0f,
                 float u0 = 0.0f, float v0 = 0.0f, float us = 1.0f, float vs = 1.0f) {
    setCommonUniforms(r, alpha, 0);
    gShader->setInt("uUseTexture", 1);
    gShader->setVec3("uTint", tr, tg, tb);
    gShader->setVec4("uUVRect", u0, v0, us, vs);
    tex.Bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
```

Jedno crtanje bi inače bilo pet do sedam linija `set...` i `glDrawElements`. `drawTexture` to radi jednim pozivom:
1. `setCommonUniforms` postavi `uPos`, `uSize` (iz `Rect`-a), `uAlpha` i `uShape`,
2. postavi `uUseTexture`, `uTint` i `uUVRect`,
3. veže teksturu,
4. **tek tada crta** (`glDrawElements`).

Parametri posle `Rect`-a imaju podrazumevane vrednosti (neprovidno, bez nijanse, cela slika), pa se najčešće piše samo:

```cpp
drawTexture(texBody, body);
```

`drawColor` radi isto, ali sa jednobojnom bojom umesto slike (`uUseTexture = 0`) i sa izborom oblika (`uShape`). Koristi se za sjenku, a kasnije za sjaj i LED.

`gShader` je globalni pokazivač na šejder program (`gShader = &shaderProgram;`), jer `shaderProgram` živi u `main`, a pomoćne funkcije su van nje.

## Raspored mašine

### Dva koordinatna sistema

Slika mašine (`machine_body.png`) je 900x1000 piksela, a ekran može biti bilo koje veličine. Zato postoje dva koordinatna sistema:

1. **Koordinate mašine:** pikseli na slici `machine_body.png`. (0, 0) je gornji levi ugao slike, a y raste nadole, kao u svakom programu za slike. Svaki deo mašine se zadaje ovde, npr. "vitrina je od (50, 130) do (850, 400)". Python skripta je mašinu crtala u istim koordinatama, pa se brojevi poklapaju sa slikom.
2. **Koordinate ekrana:** pikseli ekrana koje koristi `drawTexture`. (0, 0) je donji levi ugao, a y raste nagore.

### `updateLayout`: veličina i položaj mašine

```cpp
void updateLayout() {
    machineScale = screenH * 0.94f / MACHINE_H;
    float machineWidth = MACHINE_W * machineScale;
    machineLeft = (std::max)(screenW * 0.02f, screenW * 0.40f - machineWidth / 2.0f);
    machineTop = screenH / 2.0f + MACHINE_H * machineScale / 2.0f;
}
```

Poziva se jednom, posle `glViewport`:
- **`machineScale`:** mašina zauzima 94% visine ekrana. Skala kaže koliko piksela ekrana odgovara jednom pikselu slike.
- **`machineLeft`:** centar mašine je na 40% širine ekrana (levo od sredine, da desno ostane mesta za novčanik), ali nikad bliže od 2% levoj ivici. `std::max` bira veći od ta dva broja, pa mašina na uskom ekranu ne izlazi preko ivice.
- **`machineTop`:** mašina je vertikalno centrirana, a ovo je y njene gornje ivice na ekranu.

| | Ekran 1920x1080 | Ekran 1366x768 |
|---|---|---|
| `machineScale` | 1.015 | 0.722 |
| visina mašine | 1015 px | 722 px |
| širina mašine | 914 px | 650 px |
| visina kao % ekrana | 94% | 94% |

**Rezolucija:** u pikselima je mašina na manjem ekranu manja, ali zauzima isti deo ekrana. **Proporcije** se čuvaju jer se širina i visina množe istom skalom.

### `toScreenX`, `toScreenY` i `machineRect`

```cpp
float toScreenX(float mx) { return machineLeft + mx * machineScale; }
float toScreenY(float my) { return machineTop - my * machineScale; }

Rect machineRect(float x0, float y0, float x1, float y1) {
    return Rect{ toScreenX((x0 + x1) / 2.0f), toScreenY((y0 + y1) / 2.0f),
                 (x1 - x0) * machineScale, (y1 - y0) * machineScale };
}
```

- `toScreenX` i `toScreenY` prevode tačku sa slike mašine na ekran. Kod y se **oduzima**, jer na slici y raste nadole, a na ekranu nagore.
- `machineRect` prima gornji levi `(x0, y0)` i donji desni `(x1, y1)` ugao dela mašine **na slici** i vraća `Rect` (centar i veličinu) **na ekranu**.

Primer: sredina slike (450, 500) na ekranu 1920x1080 pada u tačku (768, 540).

### Sjenka

```cpp
Rect body = machineRect(0.0f, 0.0f, MACHINE_W, MACHINE_H);
drawColor(Rect{ body.cx + 16.0f * s, body.cy - 30.0f * s, body.w - 40.0f * s, body.h - 40.0f * s }, 0.0f, 0.0f, 0.0f, 0.3f);
drawTexture(texBody, body);
```

Sjenka je vizuelni trik: bez nje mašina izgleda kao nalepljena na zid, a tamni pojas desno i ispod nje daje utisak da stoji ispred zida. To je crni pravougaonik sa alfom 0.3, nacrtan **pre** mašine:
- pomeren **udesno** (`+16`) i **nadole** (`-30`), jer je zamišljeno svetlo gore levo,
- **providan**, pa zid ispod samo potamni,
- **malo manji** od mašine, da njegovi oštri uglovi ne vire iza zaobljenih uglova mašine,
- pomaci su pomnoženi skalom (`s = machineScale`), pa je sjenka u istom odnosu prema mašini na svakom ekranu.

Mašina se crta preko sjenke i prekrije je, osim tankog pojasa desno i ispod.

## Natpis sa imenom

```cpp
float margin = screenH * 0.02f;
float h = screenH * 0.045f;
float w = h * texName.width / (float)texName.height;
float maxW = screenW - toScreenX(MACHINE_W) - 2.0f * margin;
if (w > maxW && maxW > 0.0f) { w = maxW; h = w * texName.height / (float)texName.width; }
drawTexture(texName, Rect{ screenW - margin - w / 2.0f, screenH - margin - h / 2.0f, w, h }, 0.75f);
```

- Natpis je u gornjem desnom uglu, sa marginom od 2% visine ekrana.
- Visina mu je 4,5% visine ekrana, a širina se računa iz proporcija slike (`width / height`), pa se tekst ne razvlači.
- Ako desno od mašine nema dovoljno mesta (uzak ekran), natpis se smanji.
- Crta se sa `uAlpha = 0.75`, pa je poluprovidan, kako zahteva specifikacija. Slika natpisa već ima poluprovidnu tamnu pozadinu.
- Crta se posle mašine, da bude iznad svega.

## Kursor

```cpp
cursorBottle = loadImageToCursor("res/cursor.png", 20, 3);
glfwSetCursor(window, cursorBottle);
...
glfwDestroyCursor(cursorBottle);
```

Kursor je **sistemski kursor** napravljen od slike preko `glfwCreateCursor`, kao na vežbama, a ne tekstura nacrtana na poziciji miša. Slika je bočica parfema (40x80 piksela), a tačka klika (20, 3) je vrh čepa. Specifikacija traži kursor povezan sa temom (kovanica, kartica ili bočica). Kasnije se, kad korisnik uzme kovanicu ili karticu, kursor menja u njih (`cursor_coin.png`, `cursor_card.png`).

Kursor se pravi pre `glewInit()`, što je u redu, jer `glfwCreateCursor` ne koristi OpenGL.

## Grafika (folder `res`)

Sve slike su generisane Python skriptom (biblioteka Pillow) i nalaze se u folderu `res/`. Slike su PNG sa alfa kanalom, pa su delovi oko oblika providni.

Tekst (nazivi i opisi parfema, natpisi) je nacrtan direktno u slikama, a brojevi se crtaju iz atlasa cifara `digits.png`, pa FreeType nije potreban.

`cursor_coin.png` (64x64) i `cursor_card.png` (96x60) su umanjene kopije `coin.png` i `card.png`, jer sistemski kursor prikazuje sliku u njenoj stvarnoj veličini.

## Dodavanje novog elementa

Svaki novi deo mašine dodaje se na isti način:

**1. Nađi gde je element na slici `machine_body.png`**, u pikselima slike: gornji levi `(x0, y0)` i donji desni `(x1, y1)` ugao.

**2. Učitaj teksturu**, pre petlje:
```cpp
Texture texShowcase("res/showcase.png", GL_TEXTURE_2D, GL_TEXTURE0);
```

**3. Napravi funkciju koja vraća njegov `Rect`**, preko `machineRect`:
```cpp
Rect showcaseRect() { return machineRect(50.0f, 130.0f, 850.0f, 400.0f); }
```

**4. Nacrtaj ga u petlji**, na pravom mestu u redosledu (od pozadine ka napred):
```cpp
drawTexture(texShowcase, showcaseRect());
```

**5. Obriši teksturu na kraju:**
```cpp
texShowcase.Delete();
```

Kad element treba da reaguje na klik, isti `Rect` se koristi i za proveru: `showcaseRect().contains(mouseX, mouseY)`. Pošto je `Rect` jedan, ono što se vidi i ono na šta se klikne uvek se poklapaju.

Elementi van mašine (natpis sa imenom, novčanik) ne idu preko `machineRect`, nego se zadaju direktno u pikselima ekrana, relativno na `screenW` i `screenH`.

## Brisanje resursa

```cpp
texBackground.Delete();
texBody.Delete();
texName.Delete();

VAO1.Delete();
VBO1.Delete();
EBO1.Delete();
shaderProgram.Delete();

glfwDestroyCursor(cursorBottle);

glfwDestroyWindow(window);
glfwTerminate();
```

Prvo se brišu OpenGL objekti (teksture, baferi, šejder program), dok kontekst još postoji, zatim kursor, pa prozor i GLFW. Svaki resurs koji je napravljen na početku ima svoje brisanje na kraju.