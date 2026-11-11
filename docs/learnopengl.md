# Teorija: osnove OpenGL-a

Ovaj fajl sadrži teoriju na kojoj se zasniva projekat. Kako je ta teorija primenjena u samom projektu i zašto je implementacija baš takva, opisano je u [README.md](../README.md).

## OpenGL

OpenGL nije biblioteka sa gotovim kodom, već **specifikacija**: dokument koji propisuje šta svaka funkcija treba da radi (ime, parametri, rezultat), ali ne i kako. Stvarnu implementaciju pišu proizvođači grafičkih kartica (NVIDIA, AMD, Intel) u svojim drajverima. **Kada moj C++ kod pozove OpenGL funkciju, poziv stiže do drajvera, koji ga prevodi u komande koje razume konkretan GPU.** Implementacije se mogu razlikovati od proizvođača do proizvođača, ali rezultat mora biti onakav kako specifikacija propisuje. Zato su čudno ponašanje i greške u OpenGL-u obično greške drajvera i rešavaju se njegovim ažuriranjem.

Drajver je program proizvođača grafičke kartice instaliran na operativnom sistemu, onaj koji se instalira sa sajta AMD-a, NVIDIA-e ili Intela, ili ga sistem instalira sam. On jedini zna kako da priča sa konkretnom grafičkom karticom.

## GLFW

GLFW je biblioteka napisana u C-u koja OpenGL-u daje ono što on sam nema: **prozor**. OpenGL ne zna šta je prozor, tastatura ili miš. Zna samo da crta u kontekst koji mu neko obezbedi.

GLFW:
- pravi prozor i OpenGL kontekst vezan za njega,
- čita tastaturu i miš (preko callback funkcija ili `glfwGetKey`),
- meri vreme (`glfwGetTime`).

Pre pravljenja prozora GLFW-u se kaže kakav kontekst treba:

```cpp
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
```

Traži se OpenGL 3.3. Ako grafička kartica ne podržava tu verziju, `glfwCreateWindow` vraća `NULL`, umesto da se program kasnije sruši na nepostojećoj funkciji. **Core profil** znači samo moderni deo OpenGL-a, bez starih funkcija kao `glBegin`/`glEnd`, pa se crta isključivo kroz šejdere. To je **programabilni pajplajn**.

## GLEW

GLEW je biblioteka napisana u C-u koja **u toku rada programa pronalazi OpenGL funkcije u drajveru** i omogućava da ih pozivam kao obične funkcije.

OpenGL je samo specifikacija, a kod funkcija je u drajveru grafičke kartice. Zato adrese većine funkcija (npr. `glCreateShader`, `glGenBuffers`) nisu poznate kad se program kompajlira, već moraju da se pronađu kad program radi. Na Windowsu sistem sam po sebi nudi samo stari OpenGL 1.1, a sve novije se traži od drajvera (`wglGetProcAddress`). GLEW to radi jednim pozivom:

```cpp
if (glewInit() != GLEW_OK) return endProgram("Failed to initialize GLEW.");
```

`glewInit()` pronađe adrese svih funkcija koje drajver podržava i poveže ih sa imenima iz `<GL/glew.h>`. Bez toga bi poziv bilo koje moderne OpenGL funkcije srušio program.

Dva pravila:
- **`glewInit()` ide posle `glfwMakeContextCurrent`**, jer se adrese traže za konkretan kontekst, a pre bilo koje druge OpenGL funkcije.
- **`#include <GL/glew.h>` ide pre `#include <GLFW/glfw3.h>`**, inače GLFW uključi sistemski OpenGL header, a GLEW javi grešku da je OpenGL već uključen.

GLEW omogućava i proveru da li kartica podržava neku ekstenziju (`glewIsSupported`). GLAD radi isti posao kao GLEW.

**Ukratko:** GLFW pravi prozor i kontekst, a GLEW omogućava da se u tom kontekstu uopšte pozivaju moderne OpenGL funkcije.

## Mašina stanja (kontekst)

OpenGL je mašina stanja: **pamti svako podešavanje koje mu zadaš** i sve naredne naredbe rade prema tom podešavanju, dok ga ne promeniš. Sve što OpenGL trenutno pamti zove se **kontekst**. Kontekst je vezan za jedan prozor i pravi ga GLFW.

Zato OpenGL funkcije spadaju u dve grupe:
- **funkcije koje menjaju stanje**: samo nešto podese i ništa vidljivo ne urade, npr. `glClearColor`, `glBindBuffer`, `glUseProgram`,
- **funkcije koje koriste stanje**: rade na osnovu onoga što je trenutno podešeno, npr. `glClear` prefarba ekran bojom koja je postavljena, a `glDrawElements` crta sa trenutno aktivnim šejderom i VAO-om.

Posledica: **redosled poziva je bitan**. Ista naredba daje različit rezultat u zavisnosti od toga šta je pre nje podešeno.

## Objekti, id i tabela

Objekat u OpenGL-u je paket podataka i podešavanja oko jedne teme, npr. bafer sa verteksima (VBO), opis atributa (VAO), tekstura ili šejder program. **Objekat drži drajver, a ne moj program.** Ja nikad ne dobijem sam objekat ni njegovu adresu.

Kad napravim objekat:

```cpp
GLuint VBO;
glGenBuffers(1, &VBO);
```

drajver napravi objekat i kod sebe zapiše vezu **id → objekat**, kao red u tabeli. U moju promenljivu upiše **samo id**, običan ceo broj (npr. 1, 2, 3). Zato prosleđujem adresu promenljive (`&VBO`), da bi drajver imao gde da upiše broj. Prvi parametar (`1`) je koliko objekata pravim.

Kad god kasnije pošaljem taj broj, drajver ga potraži u svojoj tabeli i stigne do pravog objekta. Id je kao broj garderobe: ja imam samo cedulju, a stvar čuva garderober.

Id `0` je rezervisan i znači "nijedan objekat".

## Slotovi i vezivanje

Kontekst ima **slotove**, polja u kojima piše id trenutno vezanog objekta određene vrste. Slotovi imaju unapred definisana imena:
- `GL_ARRAY_BUFFER` za bafer sa verteksima (VBO),
- `GL_ELEMENT_ARRAY_BUFFER` za bafer sa indeksima (EBO),
- `GL_TEXTURE_2D` za 2D teksturu.

Vezivanje upisuje id objekta u slot:

```cpp
glBindBuffer(GL_ARRAY_BUFFER, VBO);
```

Od tog trenutka sve funkcije koje rade preko tog slota menjaju baš taj objekat:

```cpp
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
```

`glBufferData` ne prima id bafera, nego ime slota. Pogleda koji je id upisan u `GL_ARRAY_BUFFER` i kopira podatke u taj objekat. **Ovako se u OpenGL-u radi sa objektima: ne menjaš objekat direktno, nego ga vežeš za slot i onda radiš preko slota.**

### Razvezivanje ne briše ništa

```cpp
glBindBuffer(GL_ARRAY_BUFFER, 0);
```

U slot se upiše 0, pa slot više ne pokazuje ni na šta. Objekat i dalje postoji kod drajvera, sa svim podacima i podešavanjima, jer se ona čuvaju **u objektu, a ne u slotu**. Kad ga ponovo vežem, sve je tu. Razvezivanje služi da neka kasnija naredba slučajno ne izmeni objekat.

Objekat stvarno nestaje tek sa `glDelete...`, npr. `glDeleteBuffers(1, &VBO)`.

### Zašto je to korisno

Više objekata podesim jednom, na početku. Kasnije pre crtanja samo vežem onaj koji mi treba, bez ponovnog podešavanja.

### Lanac ukratko

1. **Kontekst** je sve što OpenGL pamti.
2. **Objekat** je paket podataka koji drži drajver, a ne moj program.
3. **Id** je broj koji dobijem umesto objekta. Drajver u svojoj tabeli zna koji objekat odgovara kom id-u.
4. **Slot** je mesto u kontekstu u kome piše id trenutno vezanog objekta.
5. **Vezivanje** (`glBind...`) upiše id u slot, a funkcije koje rade preko slota menjaju taj objekat.
6. **Razvezivanje** (`glBind...(..., 0)`) samo isprazni slot. Objekat ostaje, a briše se tek sa `glDelete...`.

## Prozor i render petlja

### Kontekst

```cpp
glfwMakeContextCurrent(window);
```

Postavlja kontekst prozora kao aktivan. **Bez aktivnog konteksta nijedna OpenGL funkcija ne radi.**

### Viewport

```cpp
glViewport(0, 0, width, height);
```

Kaže OpenGL-u u koji deo prozora crta: donji levi ugao (0, 0), pa širina i visina u pikselima. Viewport preslikava koordinate od -1 do 1 u piksele prozora. Ako prozor menja veličinu, viewport treba ažurirati preko `glfwSetFramebufferSizeCallback`.

### Render petlja

```cpp
while (!glfwWindowShouldClose(window))
{
    glClear(GL_COLOR_BUFFER_BIT);
    // crtanje
    glfwSwapBuffers(window);
    glfwPollEvents();
}
```

- Jedan prolaz kroz petlju je jedan **frame**, jedna kompletna slika. Kad se slike brzo smenjuju, oko ih vidi kao kretanje.
- `glfwWindowShouldClose` proverava da li je traženo zatvaranje.
- `glfwPollEvents` obrađuje događaje (tastatura, miš) i poziva registrovane callback-e.
- `glfwSwapBuffers` menja zadnji i prednji bafer.
- Sve što se crta ide između `glClear` i `glfwSwapBuffers`.

### Dvostruki bafer (double buffering)

- **Prednji bafer** je ono što se trenutno vidi na ekranu.
- **Zadnji bafer** je onaj u koji se crta.
- Kad je crtanje gotovo, baferi se zamene, pa se slika prikazuje tek kad je cela gotova. Time se izbegava treperenje, jer se slika crta deo po deo.

Bafer je slika u memoriji grafičke kartice: tabela piksela veličine ekrana, gde za svaki piksel piše njegova boja (RGBA).

### Brisanje ekrana

```cpp
glClearColor(0.2f, 0.2f, 0.25f, 1.0f); // menja stanje
glClear(GL_COLOR_BUFFER_BIT);          // koristi stanje
```

- `glClearColor` samo postavlja boju (RGBA).
- `glClear` **prefarba svaki piksel** zadnjeg bafera tom bojom. "Brisanje" ovde ne znači da piksel postaje prazan, jer piksel uvek ima neku boju, nego da se cela slika prefarba jednom bojom, kao kad se okreči zid.
- Briše se na početku svakog frame-a, inače bi se nova slika crtala preko stare i ostajali bi tragovi.
- `GL_COLOR_BUFFER_BIT` znači da se briše bafer boja. Postoje i drugi, npr. dubinski za 3D (`GL_DEPTH_BUFFER_BIT`).

### Callback i polling

```cpp
void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

glfwSetKeyCallback(window, keyboardCallback);
```

- **Callback** je moja funkcija koju GLFW sam pozove kad se desi događaj. Registrujem je jednom preko `glfwSet...Callback`, a GLFW je poziva iz `glfwPollEvents()`.
- Potpis mora biti tačno onakav kakav GLFW očekuje, čak i kad neke parametre ne koristim. Ime funkcije je slobodno.
- `action` može biti `GLFW_PRESS`, `GLFW_RELEASE` ili `GLFW_REPEAT`.
- `glfwSetWindowShouldClose` ne gasi program odmah. Samo postavlja zastavicu, a petlja je proveri i završi se, pa program uvek prolazi kroz čišćenje na kraju.

Druga mogućnost je **polling**: funkcija koju ja zovem svaki frame i koja pita `glfwGetKey` da li je taster trenutno pritisnut. Callback se izvršava samo kad se događaj zaista desi, a polling svaki frame. U oba slučaja `glfwPollEvents()` mora da se zove svaki frame.

### V-sync

**V-sync** (vertikalna sinhronizacija) tera `glfwSwapBuffers` da čeka da monitor završi osvežavanje, pa tek onda zameni sliku. Time se izbegava **tearing** (vidljiva linija gde se gornji deo slike razlikuje od donjeg), ali je broj frame-ova po sekundi vezan za frekvenciju monitora (60 Hz, 144 Hz...).

```cpp
glfwSwapInterval(1);  // v-sync uključen
glfwSwapInterval(0);  // v-sync isključen
```

## Grafički pajplajn

Grafički pajplajn pretvara koordinate verteksa u obojene piksele na ekranu, kroz niz koraka, gde svaki korak kao ulaz prima izlaz prethodnog:

1. **Vertex šejder** prima jedan verteks i određuje njegovu poziciju na ekranu.
2. **Sklapanje primitiva** spaja verteksе u zadati oblik.
3. **Rasterizacija** preslikava oblike na piksele ekrana i pravi **fragmente**. Fragment su svi podaci potrebni da se iscrta jedan piksel.
4. **Fragment šejder** računa konačnu boju svakog fragmenta.
5. **Blending** meša tu boju sa onim što je već na ekranu, u zavisnosti od alfa vrednosti (providnosti).

Koraci se izvršavaju na grafičkoj kartici, paralelno, na hiljadama malih jezgara. Programi koji se izvršavaju u tim koracima zovu se **šejderi**. U modernom OpenGL-u vertex i fragment šejder moramo da napišemo sami, jer podrazumevani ne postoje.

### Primitivi

**Primitivi** su osnovni oblici koje OpenGL zna da nacrta: tačke, linije i trouglovi. Prvi argument funkcije za crtanje kaže kako da se verteksi grupišu u oblike:

| Primitiv | Kako grupiše verteksе 0, 1, 2, 3 |
|---|---|
| `GL_POINTS` | 4 tačke |
| `GL_LINES` | linije 0–1 i 2–3 |
| `GL_LINE_STRIP` | 0–1–2–3 |
| `GL_LINE_LOOP` | 0–1–2–3–0 |
| `GL_TRIANGLES` | trougao 0-1-2 (svaka tri verteksa su jedan trougao) |
| `GL_TRIANGLE_STRIP` | 0-1-2 i 1-2-3 |
| `GL_TRIANGLE_FAN` | 0-1-2 i 0-2-3 (svi dele prvi verteks) |

GPU je napravljen da crta trouglove, jer tri tačke uvek leže u jednoj ravni. Zato u core profilu ne postoje `GL_QUADS` ni `GL_POLYGON`: pravougaonik se crta kao dva trougla.

### Normalizovane koordinate uređaja (NDC)

Posle vertex šejdera pozicije moraju biti u opsegu od -1 do 1 po x i y. Sve van tog opsega se odseca. (0, 0) je centar ekrana, a y raste nagore, za razliku od koordinata prozora i miša, gde je (0, 0) gornji levi ugao i y raste nadole. Viewport zatim NDC preslikava u piksele.

## Ulaz verteksa

**Verteks** je skup podataka za jednu tačku. Podaci verteksa su podeljeni u **atribute** (npr. pozicija, boja, koordinata teksture).

### VBO (Vertex Buffer Object)

VBO je memorija na grafičkoj kartici u kojoj stoje podaci o verteksima. Slanje podataka sa procesora na karticu je sporo, pa se pošalju jednom i ostanu tamo, a vertex šejder im tamo pristupa gotovo trenutno.

```cpp
glGenBuffers(1, &ID);
glBindBuffer(GL_ARRAY_BUFFER, ID);
glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
```

- `glGenBuffers` napravi bafer i u `ID` upiše njegov id.
- `glBindBuffer` veže bafer za slot `GL_ARRAY_BUFFER`.
- `glBufferData` kopira podatke u trenutno vezani bafer. Drugi argument je veličina u bajtovima.
- Četvrti argument kaže kartici kako će se podaci koristiti:
  - `GL_STREAM_DRAW`: postave se jednom, koriste se nekoliko puta,
  - `GL_STATIC_DRAW`: postave se jednom, koriste se mnogo puta,
  - `GL_DYNAMIC_DRAW`: često se menjaju i koriste mnogo puta.

### EBO (Element Buffer Object)

EBO čuva **indekse**, odnosno koji verteksi čine koji trougao. Pravougaonik se crta kao dva trougla: bez indeksa bi trebalo 6 verteksa, od kojih su dva duplikati, a sa indeksima su dovoljna 4. To se zove **indeksirano crtanje**.

```cpp
GLuint indices[] = {
    0, 1, 2,   // prvi trougao
    0, 2, 3,   // drugi trougao
};
```

Brojevi u `indices` su **redni brojevi verteksa** u nizu verteksa (brojanje počinje od 0), a ne koordinate:

```
   3 ─────────── 2
   │           ╱ │
   │  drugi  ╱   │
   │       ╱     │
   │     ╱ prvi  │
   │   ╱         │
   0 ─────────── 1
```

EBO se pravi isto kao VBO, samo je slot `GL_ELEMENT_ARRAY_BUFFER`.

### VAO (Vertex Array Object)

VAO ne čuva same podatke (to radi VBO), već **pravila** o tome kako se oni čitaju. Za svaki atribut pamti:
- id VBO-a iz kog se čitaju podaci,
- broj komponenti (npr. 2 za `vec2`),
- tip podataka (`GL_FLOAT`),
- stride i offset,
- da li je atribut uključen.

Pored toga pamti i EBO. Core OpenGL ne crta ništa bez vezanog VAO-a.

```cpp
glVertexAttribPointer(layout, numComponents, type, GL_FALSE, stride, offset);
glEnableVertexAttribArray(layout);
```

`glVertexAttribPointer` parametri:
- `layout`: koji atribut, odnosno `location` u vertex šejderu,
- `numComponents`: koliko brojeva ima atribut,
- `type`: tip brojeva,
- `GL_FALSE`: brojevi se ne normalizuju (bitno samo za cele brojeve),
- `stride`: razmak od početka jednog verteksa do početka sledećeg, u bajtovima,
- `offset`: gde unutar verteksa počinje ovaj atribut, u bajtovima. Tipa je `void*`, otuda cast.

`glEnableVertexAttribArray` uključuje atribut. Atributi su podrazumevano isključeni.

### Stride i offset

Primer: verteks sa pozicijom (x, y) i koordinatom teksture (u, v), sve `float`:

```
bajt:    0       4       8       12      16      20      24      28      32
         ┌───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┐
         │   x   │   y   │   u   │   v   │   x   │   y   │   u   │   v   │
         └───────┴───────┴───────┴───────┴───────┴───────┴───────┴───────┘
         ├──── aPos ─────┤──── aTex ─────┤──── aPos ─────┤──── aTex ─────┤
         ├────────── verteks 0 ──────────┤────────── verteks 1 ──────────┤
```

- Svaki broj je `float`, odnosno 4 bajta.
- Jedan verteks ima 4 broja: `4 * 4 = 16` bajtova. To je **stride**.
- Pozicija počinje na bajtu 0 svakog verteksa: **offset 0**.
- Koordinata teksture počinje posle `x` i `y`, na bajtu 8: **offset 8**, odnosno `2 * sizeof(float)`.

GPU za verteks `i` čita atribut sa adrese `offset + i * stride`:

```
aPos:  0 + i * 16   →  bajt 0, 16, 32, 48
aTex:  8 + i * 16   →  bajt 8, 24, 40, 56
```

### Kako VBO komunicira sa šejderom

VBO i vertex šejder nemaju direktnu vezu u kodu. Spaja ih broj lokacije:

- u C++-u `glVertexAttribPointer(0, ...)` opisuje podatke za lokaciju 0,
- u šejderu `layout (location = 0) in vec2 aPos;` prima podatke sa lokacije 0.

Pri crtanju se atributi iz vezanog VAO-a šalju ulazima šejdera sa istim `location` brojem.

> [!NOTE]
> **Kako VAO pamti VBO i EBO**
>
> VAO pamti i VBO i EBO, ali ih **dobija na dva različita načina**. Razlog je u tome gde se nalaze slotovi za ova dva bafera.
>
> ```
> KONTEKST
> ├── slot GL_ARRAY_BUFFER   = id VBO-a        ← u kontekstu, VAO ga ne pamti
> └── trenutno vezani VAO
>         │
>         ▼
>     VAO
>     ├── atribut 0: { iz kog VBO-a, broj komponenti, tip, stride, offset, uključen }
>     ├── atribut 1: { iz kog VBO-a, broj komponenti, tip, stride, offset, uključen }
>     └── slot GL_ELEMENT_ARRAY_BUFFER = id EBO-a   ← ovaj slot je deo VAO-a
> ```
>
> **VBO se prepisuje u trenutku `glVertexAttribPointer`.** Samo vezivanje VBO-a upiše njegov id u slot `GL_ARRAY_BUFFER` u kontekstu i ne menja VAO. VAO saznaje za VBO tek u `glVertexAttribPointer`: funkcija pogleda koji id piše u `GL_ARRAY_BUFFER` i **prepiše ga** u opis atributa u VAO-u. Posle toga razvezivanje VBO-a ne smeta, jer VAO već ima svoju kopiju id-a.
>
> **EBO se upisuje samim vezivanjem.** Pošto je slot `GL_ELEMENT_ARRAY_BUFFER` deo vezanog VAO-a, `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ...)` dok je VAO vezan odmah upisuje EBO u taj VAO. Zato VAO mora biti vezan **pre** nego što se veže EBO, i razvezuje se **pre** EBO-a: razvezivanje EBO-a dok je VAO vezan upisalo bi 0 u VAO, pa bi ostao bez indeksa.
>
> **Zašto je napravljeno različito:** VBO se pamti po atributu, jer različiti atributi mogu da čitaju iz različitih VBO-ova (npr. pozicije iz jednog bafera, boje iz drugog). Svaki atribut tada ima svoj VBO, svoj stride i svoj offset, a jedino što ih povezuje je isti redni broj verteksa `i`. EBO je jedan za ceo VAO, jer jedno crtanje koristi jedan niz indeksa.

> [!NOTE]
> **Ponovno vezivanje VAO-a i crtanje**
>
> VAO ne kopira podatke iz VBO-ova. Za svaki atribut pamti samo **uputstvo**: iz kog VBO-a čita, od kog bajta počinje (offset) i koliko bajtova preskače od verteksa do verteksa (stride).
>
> Pri ponovnom vezivanju ništa se ne kopira i ništa se ne podešava ponovo. U kontekstu postoji slot "trenutni VAO", a `glBindVertexArray(id)` samo upiše id u taj slot. Sve što je VAO zapamtio i dalje stoji u njemu, od trenutka kad je podešen. Funkcije za crtanje uvek gledaju trenutni VAO, pa su sva ta podešavanja od tog trenutka aktivna. Zato izgleda kao da se "sve desi odjednom": promeni se samo jedan broj.
>
> Tek u `glDrawElements` se podaci stvarno koriste. Redom:
>
> 1. pogleda trenutni program, tj. koji šejderi se izvršavaju,
> 2. pogleda trenutni VAO,
> 3. iz VAO-a uzme EBO i pročita indekse,
> 4. za svaki indeks `i` i svaki uključeni atribut izračuna odakle da čita: `VBO tog atributa + offset + i * stride`,
> 5. pročitane vrednosti pošalje u ulaze vertex šejdera sa istim `location` brojem i pokrene ga za taj verteks,
> 6. verteksе sklopi u primitive, pa slede rasterizacija i fragment šejder, koji pikselima određuje boju.
>
> VBO i EBO su u memoriji grafičke kartice od trenutka kad su napravljeni. Sam oblik se ne pamti nigde: iz istih podataka se pri svakom crtanju iznova dobijaju obojeni pikseli u zadnjem baferu, a sledeći frame se sve prefarba i crta ponovo.
>
> **VAO je po obliku, ne po objektu.** Svi objekti koji dele isti oblik koriste isti VAO, dok se međusobno razlikuju po teksturama i uniformama. Novi VAO je potreban samo za objekte sa potpuno drugačijom geometrijom, odnosno drugačijim verteksima.

## Šejderi

Šejderi su mali programi koji se izvršavaju na grafičkoj kartici. Pišu se u jeziku **GLSL**, koji liči na C. Svaki šejder počinje deklaracijom verzije (`#version 330 core` odgovara OpenGL-u 3.3), zatim slede ulazi, izlazi, uniforme i funkcija `main`.

GLSL ima vektorske tipove `vec2`, `vec3` i `vec4` (2, 3 ili 4 float vrednosti). Komponentama se pristupa preko `.x .y .z .w` ili `.r .g .b .a`, a mogu se i kombinovati (swizzling), npr. `v.xy` ili `v.zw`.

GLSL ne kompajlira Visual Studio, nego drajver, u toku rada programa. Zato VS ne prijavljuje greške u šejderima, nego ih program mora sam ispisati.

### Vertex šejder

Izvršava se jednom za svaki verteks. Ulazi su mu atributi (`in`, sa `layout (location = ...)`), a u ugrađenu promenljivu `gl_Position` (tipa `vec4`) mora da upiše gde je verteks, u NDC koordinatama. Za 2D je z = 0, a w = 1.

Kada definišem `out` promenljivu, moram joj dodeliti vrednost u `main`, da bi se prosledila dalje.

### Fragment šejder

Izvršava se za svaki fragment (piksel) koji oblik pokriva. Ima jedan izlaz tipa `vec4`, konačnu boju piksela (RGBA, svaka komponenta od 0 do 1, alfa 1 je potpuno neprovidno). `discard` odbacuje fragment, pa se za njega ništa ne upisuje.

### Veza vertex i fragment šejdera

Fragment šejder se ne povezuje preko VAO-a ni preko `location`, nego preko **imena i tipa**: `out vec2 texCoord` u vertex šejderu i `in vec2 texCoord` u fragment šejderu su ista promenljiva. To povezivanje radi **linker** pri pravljenju shader programa (`glLinkProgram`). Ako se ime ili tip ne poklapaju, dobija se greška pri linkovanju.

Dva različita spajanja:
1. **VBO → vertex šejder**: preko `location` i VAO-a.
2. **Vertex šejder → fragment šejder**: preko `out`/`in` promenljivih sa istim imenom i tipom.

---

### Interpolacija

#### Problem

Vertex šejder se izvršava samo za **verteksе**. Kod pravougaonika su to 4 verteksa, pa se vertex šejder pokrene 4 puta. Fragment šejder se izvršava za **svaki piksel** unutar oblika, što za sliku od 270x270 piksela znači više od 70.000 puta.

Vertex šejder izračuna izlazne vrednosti (`out`) samo za ta 4 ugla. Fragment šejderu, međutim, treba vrednost za **svaki** piksel. Pitanje je odakle pikselu u sredini kvadrata vrednost, kad niko nije izračunao ništa za tu tačku.

#### Rešenje

Tu vrednost računa **rasterizacija**, korak između vertex i fragment šejdera. Za svaki fragment ona pogleda koliko je udaljen od verteksa i od njihovih vrednosti napravi **mešavinu**: što je fragment bliži nekom verteksu, to više dobija od njegove vrednosti. To se zove **interpolacija**.

Za svaku `out` promenljivu vertex šejdera, fragment šejder kroz istoimenu `in` promenljivu dobija vrednost izračunatu baš za taj piksel, a ne vrednost nekog verteksa.

Fragment je „kandidat za piksel“: paket podataka koji rasterizacija napravi za jedno mesto na ekranu koje oblik pokriva, pre nego što se zna konačna boja.

Fragment nije isto što i piksel. 
Piksel je jedna ćelija u slici (baferu) koja se vidi na ekranu. Uvek ima boju.
Fragment je predlog boje za taj piksel, koji dolazi od jednog objekta koji se upravo crta.

#### Primer na jednoj ivici

Donja ivica kvadrata ide od verteksa 0 (`localPos.x = -0.5`) do verteksa 1 (`localPos.x = 0.5`):

```
verteks 0                                              verteks 1
   ●──────────────●──────────────●──────────────●──────────────●
 -0.5          -0.25             0            0.25            0.5
               (1/4 puta)    (pola puta)    (3/4 puta)
```

- piksel na samom levom kraju dobija -0.5 (cela vrednost verteksa 0),
- piksel na četvrtini puta dobija -0.25 (3/4 od verteksa 0 i 1/4 od verteksa 1),
- piksel na pola puta dobija 0 (pola od jednog i pola od drugog),
- piksel na desnom kraju dobija 0.5.

Vrednost se menja **ravnomerno**, kao na lenjiru.

#### Unutar trougla

Unutar trougla fragment je između **tri** verteksa, pa se meša vrednost sva tri, opet prema tome koliko je blizu kome. Fragment tačno u jednom temenu dobija samo njegovu vrednost, a fragment u težištu trougla dobija po trećinu od svakog.

Klasičan primer je trougao čija su temena crveno, zeleno i plavo. Vertex šejder prosledi boju samo za tri temena, a fragment šejder dobija mešavine, pa se ceo trougao preliva iz boje u boju. Nijedna od tih međuboja nije zadata u kodu, nastale su interpolacijom.

#### Ukratko

- Vertex šejder računa vrednosti **samo u verteksima**.
- Rasterizacija za **svaki piksel** napravi mešavinu tih vrednosti, prema udaljenosti od verteksa.
- Fragment šejder dobija vrednost **za svoj piksel** i pomoću nje bira boju.

---

### Uniforme

Uniforme i atributi služe da se podaci prebace sa procesora na grafičku karticu, ali različito:

- **Atribut** ima drugu vrednost za svaki verteks i dolazi iz VBO-a. Postoji samo u vertex šejderu.
- **Uniforma** je ista za celo jedno crtanje i postavlja se iz C++-a preko `glUniform...`. Pripada shader programu i može se koristiti u bilo kom šejderu tog programa, ali mora biti deklarisana u svakom šejderu u kome se koristi.

Uniforma zadržava vrednost dok je ne promenim.

```cpp
glUniform2f(glGetUniformLocation(program, "uPos"), 768.0f, 540.0f);
```

- `glGetUniformLocation` pronađe uniformu po imenu i vrati njenu lokaciju (broj).
- `glUniform2f` na tu lokaciju upiše vrednost.

Postfiks određuje tip: `1i` jedan int, `1f` jedan float, `2f`, `3f`, `4f` vektor od 2, 3 ili 4 floata. OpenGL je C biblioteka i nema preopterećivanje funkcija, pa za svaki tip postoji posebna funkcija.

Pronalaženje lokacije ne traži aktivan program, ali **postavljanje** traži, jer `glUniform...` upisuje u program koji je trenutno aktivan (`glUseProgram`).

Ako se uniforma deklariše, a ne koristi, kompajler je izbaci i `glGetUniformLocation` vraća -1. `glUniform` sa lokacijom -1 ne radi ništa i ne javlja grešku.

### Shader program

1. `glCreateShader` napravi prazan šejder objekat (`GL_VERTEX_SHADER` ili `GL_FRAGMENT_SHADER`), `glShaderSource` mu da izvorni kod, a `glCompileShader` ga kompajlira.
2. `glCreateProgram` napravi **shader program**, konačnu povezanu verziju šejdera. `glAttachShader` mu pridruži šejdere, a `glLinkProgram` ih poveže, spajajući izlaze jednog sa ulazima sledećeg.
3. Pojedinačni šejderi se posle linkovanja brišu (`glDeleteShader`), jer program ima svoju kopiju.
4. `glUseProgram` bira program kojim se crta.

Greške se proveravaju preko `glGetShaderiv` sa `GL_COMPILE_STATUS` i `glGetShaderInfoLog` za šejder, odnosno `glGetProgramiv` sa `GL_LINK_STATUS` i `glGetProgramInfoLog` za program.

## Providnost (blending)

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

Bez ovoga se alfa ignoriše. Sa blendingom se nova boja meša sa onom koja je već u baferu:

```
konačna boja = nova boja * alfa + boja ispod * (1 - alfa)
```

Za piksel sa alfom 0.6: 60% nove boje i 40% one ispod. Pošto se meša sa onim što je već nacrtano, **redosled crtanja je bitan**: pozadina se crta prva, a providni objekti posle onoga što treba da se vidi kroz njih. U 2D bez dubinskog bafera ono što je kasnije nacrtano je ispred.

## Teksture

**Tekstura** je 2D slika koja se lepi na oblik. Umesto da se detalji zadaju bojama verteksa, sve je u slici.

### Ukratko

1. **Slika ide na grafičku karticu.** PNG se učita (`stbi_load`) i pošalje na karticu (`glTexImage2D`). To je tekstura.
2. **Svaki ugao oblika kaže koji deo slike mu pripada.** Pored pozicije, svaki verteks ima koordinatu teksture (u, v): (0, 0) je donji levi ugao slike, (1, 1) gornji desni.
3. **Šejder za svaki piksel uzme boju iz slike.** Za piksele između uglova GPU sam izračuna koji im deo slike pripada (interpolacija), a fragment šejder sa `texture(...)` uzme boju na tom mestu.

**Koja** slika se koristi određuje `glBindTexture` pre crtanja, a ne verteksi.

### Detalji

### Koordinate teksture

Svakom verteksu se kaže kom delu slike odgovara, preko **koordinate teksture**. Ose na slici se zovu **u** (vodoravno, kao x) i **v** (uspravno, kao y). Zovu se drugačije da se ne pomešaju sa koordinatama ekrana. U OpenGL-u se iste ose zovu i **s** i **t** (otuda `GL_TEXTURE_WRAP_S` i `GL_TEXTURE_WRAP_T`).

Koordinate idu od (0, 0) za donji levi ugao slike do (1, 1) za gornji desni, bez obzira na veličinu slike u pikselima. Uzimanje boje iz slike na određenoj koordinati zove se **uzorkovanje** (sampling). Rasterizacija interpolira koordinate za svaki fragment.

Pojedinačni piksel teksture zove se **teksel**.

Tekstura sama po sebi ne zna ništa o verteksima. Vezu između oblika i slike pravi **koordinata teksture** (u, v), koja se zadaje kao poseban atribut svakog verteksa, pored pozicije. Njom se kaže koja tačka slike pripada tom verteksu, gde (0, 0) označava donji levi, a (1, 1) gornji desni ugao slike. Vertex šejder prima ovu koordinatu kao ulazni atribut i prosleđuje je fragment šejderu kroz izlaznu promenljivu. Rasterizacija je potom interpolira, pa svaki fragment unutar oblika dobija svoju koordinatu na slici, srazmerno udaljenosti od verteksa. Fragment šejder pomoću funkcije `texture(sampler, koordinata)` uzorkuje boju slike na toj koordinati, uz zadato filtriranje. Koja slika se pri tome čita ne određuju verteksi, nego stanje OpenGL-a: tekstura se pre crtanja veže za teksturnu jedinicu (`glActiveTexture` i `glBindTexture`), a uniforma tipa `sampler2D` preko `glUniform1i` dobija broj te jedinice. Zato isti verteksi mogu da se koriste sa bilo kojom slikom, jer se menja samo tekstura vezana pre crtanja.

---

### Kako tekstura "zna" za koji je verteks

#### 1. Tekstura sama po sebi ne zna ništa o verteksima.

Tekstura je samo slika u memoriji grafičke kartice: tabela teksela, bez ikakve informacije o tome na šta će se lepiti.

```cpp
unsigned int texture;
glGenTextures(1, &texture);
glBindTexture(GL_TEXTURE_2D, texture);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
```

U ovom kodu nigde se ne pominju verteksi, oblik ni pozicija. Tekstura je samo napravljena i napunjena pikselima.

#### 2. Vezu između oblika i slike pravi koordinata teksture (u, v), koja se zadaje kao poseban atribut svakog verteksa, pored pozicije.

Svaki verteks pored pozicije dobija još dva broja:

```cpp
float vertices[] = {
    // pozicija (x, y)    koordinata teksture (u, v)
    -0.5f, -0.5f,         0.0f, 0.0f,
     0.5f, -0.5f,         1.0f, 0.0f,
     0.0f,  0.5f,         0.5f, 1.0f,
};
```

To je drugi atribut, pa ga treba opisati VAO-u, na svojoj lokaciji:

```cpp
// atribut 0: pozicija
glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);
// atribut 1: koordinata teksture, počinje posle dva floata pozicije
glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
glEnableVertexAttribArray(1);
```

#### 3. Njom se kaže koja tačka slike pripada tom verteksu, gde (0, 0) označava donji levi, a (1, 1) gornji desni ugao slike.

Za trougao iz primera:

```
slika:                              trougao:
(0,1) ┌─────────●─────────┐ (1,1)            ● verteks 2 → (0.5, 1)
      │      (0.5, 1)     │                 ╱ ╲
      │                   │                ╱   ╲
      │                   │               ╱     ╲
(0,0) ●───────────────────● (1,0)        ●───────●
                                verteks 0 → (0,0)  verteks 1 → (1,0)
```

- verteks 0 (dole levo) dobija donji levi ugao slike (0, 0),
- verteks 1 (dole desno) dobija donji desni ugao slike (1, 0),
- verteks 2 (vrh) dobija sredinu gornje ivice slike (0.5, 1).

Koordinate su uvek od 0 do 1, bez obzira na to koliko je slika velika u pikselima.

#### 4. Vertex šejder prima ovu koordinatu kao ulazni atribut i prosleđuje je fragment šejderu kroz izlaznu promenljivu.

```glsl
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;   // ulaz: koordinata teksture verteksa

out vec2 TexCoord;                         // izlaz za fragment šejder

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;                  // samo je prosledi dalje
}
```

`location = 1` se poklapa sa prvim argumentom `glVertexAttribPointer(1, ...)` iz koraka 2. Vertex šejder sa koordinatom teksture ne radi ništa, samo je prenese.

#### 5. Rasterizacija je potom interpolira, pa svaki fragment unutar oblika dobija svoju koordinatu na slici, srazmerno udaljenosti od verteksa.

Vertex šejder je dao `TexCoord` samo za tri temena. Za svaki fragment unutar trougla rasterizacija napravi mešavinu te tri vrednosti:

| Fragment | Dobijeni `TexCoord` |
|---|---|
| tačno na verteksu 0 | (0, 0) |
| na sredini donje ivice, između verteksa 0 i 1 | (0.5, 0) |
| na pola puta između verteksa 0 i vrha | (0.25, 0.5) |
| u težištu trougla | otprilike (0.5, 0.33) |

Svaki fragment tako "zna" koja tačka slike mu pripada, iako je niko nije zadao.

#### 6. Fragment šejder pomoću funkcije texture(sampler, koordinata) uzorkuje boju slike na toj koordinati, uz zadato filtriranje.

```glsl
#version 330 core
in vec2 TexCoord;                     // interpolirana koordinata za ovaj fragment
out vec4 FragColor;

uniform sampler2D ourTexture;         // tekstura iz koje se čita

void main()
{
    FragColor = texture(ourTexture, TexCoord);
}
```

`in vec2 TexCoord` je povezan sa `out vec2 TexCoord` iz vertex šejdera preko istog imena i tipa. `texture` vrati boju slike na tom mestu. Kako se boja računa kad koordinata ne pada tačno na centar teksela, određuje filtriranje, podešeno pri pravljenju teksture:

```cpp
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
```

#### 7. Koja slika se pri tome čita ne određuju verteksi, nego stanje OpenGL-a: tekstura se pre crtanja veže za teksturnu jedinicu, a uniforma tipa sampler2D preko glUniform1i dobija broj te jedinice.

Jednom, pre petlje, sampler se poveže sa teksturnom jedinicom 0:

```cpp
glUseProgram(shaderProgram);
glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);   // ourTexture čita iz jedinice 0
```

Pre crtanja se tekstura veže za tu jedinicu:

```cpp
glActiveTexture(GL_TEXTURE0);              // izaberi jedinicu 0
glBindTexture(GL_TEXTURE_2D, texture);     // veži teksturu za nju
glBindVertexArray(VAO);
glDrawArrays(GL_TRIANGLES, 0, 3);
```

Lanac je: `ourTexture` → jedinica 0 → tekstura koja je u tom trenutku vezana za jedinicu 0.

#### 8. Zato isti verteksi mogu da se koriste sa bilo kojom slikom, jer se menja samo tekstura vezana pre crtanja.

```cpp
glBindVertexArray(VAO);                    // isti verteksi

glBindTexture(GL_TEXTURE_2D, textureDrvo);
glDrawArrays(GL_TRIANGLES, 0, 3);          // trougao sa slikom drveta

glBindTexture(GL_TEXTURE_2D, textureKamen);
glDrawArrays(GL_TRIANGLES, 0, 3);          // isti trougao sa slikom kamena
```

Koordinate teksture u verteksima kažu **koji deo** slike ide gde. Vezivanje kaže **koja** slika. Pošto su to dve odvojene stvari, isti oblik može dobiti bilo koju sliku samo promenom `glBindTexture`.

---

### Obmotavanje (wrapping)

Šta se dešava sa koordinatama van opsega 0 do 1:
- `GL_REPEAT`: slika se ponavlja (podrazumevano),
- `GL_MIRRORED_REPEAT`: ponavlja se kao u ogledalu,
- `GL_CLAMP_TO_EDGE`: koordinate se ograničavaju na ivicu, pa se razvlači ivični red piksela,
- `GL_CLAMP_TO_BORDER`: van opsega se daje zadata boja ivice.

### Filtriranje

Koordinate teksture ne zavise od rezolucije, pa OpenGL mora da odluči koju boju da da fragmentu koji ne pada tačno na centar teksela:
- `GL_NEAREST`: uzima teksel čiji je centar najbliži (oštri, "pikselizovani" rezultati),
- `GL_LINEAR`: meša susedne teksele po udaljenosti (glatko).

Filtriranje se zadaje posebno za uvećavanje (`GL_TEXTURE_MAG_FILTER`) i umanjivanje (`GL_TEXTURE_MIN_FILTER`).

### Mipmape

Kad se velika slika crta malo, jedan fragment pokriva više teksela, pa slika treperi i nazubljena je. **Mipmape** su niz umanjenih kopija slike, svaka duplo manja od prethodne. OpenGL za mali objekat uzima kopiju koja mu odgovara po veličini. `glGenerateMipmap` ih napravi sam.

Za umanjivanje postoje opcije sa mipmapama (`GL_NEAREST_MIPMAP_NEAREST`, `GL_LINEAR_MIPMAP_NEAREST`, `GL_NEAREST_MIPMAP_LINEAR`, `GL_LINEAR_MIPMAP_LINEAR`). Za uvećavanje se mipmape ne koriste, pa mipmap opcija kao filter za uvećavanje daje grešku `GL_INVALID_ENUM`.

### Učitavanje slike: stb_image

`stb_image.h` je biblioteka u jednom header fajlu koja učitava PNG, JPG i druge formate u niz bajtova.

Header sadrži dva dela: **deklaracije** funkcija (samo imena i parametri) i **kod** funkcija. Kod se uključuje samo ako je pre include-a definisano `STB_IMAGE_IMPLEMENTATION`:

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
```

To mora biti u **tačno jednom** `.cpp` fajlu. Svaki `.cpp` se kompajlira posebno, a linker ih na kraju spoji, pa:
- ako kod biblioteke ne postoji ni u jednom fajlu, linker javlja da funkcija ne postoji,
- ako postoji u dva fajla, linker javlja da je funkcija definisana dvaput (`LNK2005 ... already defined`).

Iz istog razloga `#define` ne sme biti u `.h` fajlu: header se uključuje u više `.cpp` fajlova.

`#include` je doslovno kopiranje teksta fajla na mesto te linije, pre kompajliranja. Ostali fajlovi uključuju `stb_image.h` bez `#define`, pa vide samo deklaracije, a linker njihove pozive poveže sa kodom iz fajla u kome je `#define`.

```cpp
stbi_set_flip_vertically_on_load(true);
unsigned char* bytes = stbi_load(path, &width, &height, &channels, 4);
```

- `stbi_load` prima putanju i popunjava širinu, visinu i broj kanala slike. Poslednji argument traži željeni broj kanala (4 = RGBA), a 0 znači "onoliko koliko slika ima".
- `stbi_set_flip_vertically_on_load(true)` okreće sliku pri učitavanju.

### Okretanje slike

Slika u memoriji je niz piksela, red po red. PNG je zapisan odozgo, pa je u memoriji **prvi red = gornji red slike**.

- **OpenGL** (`glTexImage2D`) uzima prvi red iz memorije kao **donji** red teksture (v = 0 je dole). Bez okretanja bi tekstura bila naopako, pa se za teksture slika okreće.
- **GLFW** (`glfwCreateCursor`) uzima prvi red kao **gornji** red kursora, kao i PNG. Za kursor se slika **ne okreće**.

`stbi_set_flip_vertically_on_load` je jedno podešavanje za celu biblioteku, kao prekidač koji ostaje u položaju u kome je ostavljen. Zato se pre svakog učitavanja postavi na vrednost koja tom učitavanju treba.

### Generisanje teksture

```cpp
glGenTextures(1, &ID);
glBindTexture(GL_TEXTURE_2D, ID);

glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
glGenerateMipmap(GL_TEXTURE_2D);
stbi_image_free(bytes);
```

Tekstura je OpenGL objekat kao i ostali: napravi se id, veže se za slot `GL_TEXTURE_2D`, pa sve naredne funkcije za teksture menjaju nju.

`glTexImage2D` prebaci piksele na grafičku karticu:
- `GL_TEXTURE_2D`: target,
- `0`: nivo mipmape (osnovni),
- prvi `GL_RGBA`: u kom formatu se tekstura čuva,
- `width`, `height`: veličina,
- `0`: uvek 0 (nasleđe),
- drugi `GL_RGBA` i `GL_UNSIGNED_BYTE`: format i tip izvornih podataka (4 bajta po pikselu),
- `bytes`: podaci.

Kad je slika na kartici, `stbi_image_free` oslobađa RAM.

### Sampler i teksturne jedinice

U fragment šejderu tekstura je uniforma tipa **sampler2D**, a boja se iz nje uzima funkcijom `texture`:

```glsl
uniform sampler2D tex0;
...
vec4 color = texture(tex0, texCoord);
```

`texture` uzorkuje boju slike na interpoliranoj koordinati, sa filtriranjem koje je podešeno.

Vrednost samplera nije sama tekstura, nego broj **teksturne jedinice** iz koje čita. Teksturne jedinice (`GL_TEXTURE0`, `GL_TEXTURE1`...) omogućavaju više tekstura u jednom šejderu. `glActiveTexture` bira jedinicu, a `glBindTexture` veže teksturu za trenutno aktivnu jedinicu. Sampler se poveže sa jedinicom preko `glUniform1i`:

```cpp
glUniform1i(glGetUniformLocation(program, "tex0"), 0);   // tex0 čita iz jedinice 0
```

Jedinica 0 je podrazumevana, ali je ne postavljaju svi drajveri, pa je bolje postaviti je eksplicitno.

### Providnost teksture

PNG slike imaju alfa kanal. Pikseli sa alfom 0 su potpuno providni, pa se uz blending na tim mestima vidi ono što je ispod. Tako slika kovanice izgleda okrugla, iako je nacrtana na kvadratu.

### Atlas tekstura

**Atlas** je jedna slika u kojoj je više manjih slika, poređanih jedna do druge. Umesto da svaka mala slika bude poseban fajl i posebna tekstura, sve su u jednoj, a pri crtanju se preko koordinata teksture uzme samo isečak koji treba. Prednosti: jedan fajl, jedno učitavanje i jedno vezivanje teksture. Koristi se za fontove, cifre i animacije.

## Brisanje resursa

OpenGL objekti su vezani za kontekst, pa se brišu (`glDelete...`) **pre** nego što se kontekst uništi (`glfwDestroyWindow`, `glfwTerminate`). Posle toga OpenGL poziv može da sruši program.