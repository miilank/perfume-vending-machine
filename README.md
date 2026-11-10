### 2D simulation of a perfume vending machine in C++ and OpenGL

Ovaj projekat je izrađen u OpenGL-u. OpenGL nije biblioteka sa gotovim kodom, već **specifikacija**: dokument koji propisuje šta svaka funkcija treba da radi (ime, parametri, rezultat), ali ne i kako. Stvarnu implementaciju pišu proizvođači grafičkih kartica (NVIDIA, AMD, Intel) u svojim drajverima. **Kada moj C++ kod pozove OpenGL funkciju, poziv stiže do drajvera, koji ga prevodi u komande koje razume konkretan GPU**. Implementacije se mogu razlikovati od proizvođača do proizvođača, ali rezultat mora biti onakav kako specifikacija propisuje. Zato su čudno ponašanje i greške u OpenGL-u obično greške drajvera i rešavaju se njegovim ažuriranjem.

Drajver je program proizvođača grafičke kartice instaliran na OS-u, onaj koji instaliraš sa sajta AMD-a, NVIDIA-e ili Intela, ili ga OS Update instalira sam. On jedini zna kako da priča sa konkretnom grafičkom karticom.

### GLFW

GLFW je biblioteka napisana u C-u koja OpenGL-u daje ono što on sam nema: **prozor**. OpenGL ne zna šta je prozor, tastatura ili miš. Zna samo da crta u kontekst koji mu neko obezbedi. 

GLFW:
- pravi prozor i OpenGL kontekst vezan za njega,
- čita tastaturu i miš (preko callback funkcija ili `glfwGetKey`),
- meri vreme (`glfwGetTime`).

Pre pravljenja prozora GLFW-u se kaže kakav kontekst nam treba:

```cpp
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
```

Traži se OpenGL 3.3. Ako grafička kartica ne podržava tu verziju, `glfwCreateWindow` vraća `NULL` i program se završava, umesto da se kasnije sruši na nepostojećoj funkciji. **Core profil** znači samo moderni deo OpenGL-a, bez starih funkcija kao `glBegin`/`glEnd`, pa se crta isključivo kroz šejdere.

### GLEW

GLEW je biblioteka napisana u C-u koja **u toku rada programa pronalazi OpenGL funkcije u drajveru** i omogućava da ih pozivam kao obične funkcije.

Zašto je potrebna: OpenGL je samo specifikacija, a kod funkcija je u drajveru grafičke kartice. Zato adrese većine funkcija (npr. `glCreateShader`, `glGenBuffers`) nisu poznate kad se program kompajlira, već moraju da se pronađu kad program radi. Na Windowsu sistem sam po sebi nudi samo stari OpenGL 1.1, a sve novije se traži od drajvera (`wglGetProcAddress`). GLEW to radi umesto mene, jednim pozivom:

```cpp
if (glewInit() != GLEW_OK) return endProgram("Failed to initialize GLEW.");
```

`glewInit()` pronađe adrese svih funkcija koje drajver podržava i poveže ih sa imenima iz `<GL/glew.h>`. Bez toga bi poziv bilo koje moderne OpenGL funkcije srušio program. Adrese OpenGL funkcija se traže tek u toku rada(RUNTIME), jer zavise od drajvera.

Dva pravila:
- **`glewInit()` ide posle `glfwMakeContextCurrent`**, jer se adrese traže za konkretan kontekst, a pre bilo koje druge OpenGL funkcije.
- **`#include <GL/glew.h>` ide pre `#include <GLFW/glfw3.h>`**, inače GLFW uključi sistemski OpenGL header, a GLEW javi grešku da je OpenGL već uključen.

GLEW omogućava i proveru da li kartica podržava neku ekstenziju (`glewIsSupported`), ali to u projektu ne koristimo.

**Ukratko:** GLFW pravi prozor i kontekst, a GLEW omogućava da u tom kontekstu uopšte možeš da pozivaš moderne OpenGL funkcije. GLAD radi isti posao kao GLEW.

### Mašina stanja (kontekst)

OpenGL je mašina stanja: **pamti svako podešavanje koje mu zadaš** i sve naredne naredbe rade prema tom podešavanju, dok ga ne promeniš. Sve što OpenGL trenutno pamti zove se **kontekst**. Kontekst je vezan za jedan prozor i pravi ga GLFW.

Zato OpenGL funkcije spadaju u dve grupe:
- **funkcije koje menjaju stanje**: samo nešto podese i ništa vidljivo ne urade, npr. `glClearColor`, `glBindBuffer`, `glUseProgram`,
- **funkcije koje koriste stanje**: rade na osnovu onoga što je trenutno podešeno, npr. `glClear` briše(prefarba) bojom koja je postavljena, a `glDrawElements` crta sa trenutno aktivnim šejderom i VAO-om.

Posledica: **redosled poziva je bitan**. Ista naredba daje različit rezultat u zavisnosti od toga šta je pre nje podešeno.

### Objekti, id i tabela

Objekat u OpenGL-u je paket podataka i podešavanja oko jedne teme, npr. bafer sa verteksima (VBO), opis atributa (VAO), tekstura ili šejder program. **Objekat drži drajver u memoriji grafičke kartice, a ne moj program.** Ja nikad ne dobijem sam objekat ni njegovu adresu.

Kad napravim objekat:

```cpp
GLuint VBO;
glGenBuffers(1, &VBO);
```

drajver napravi objekat i kod sebe zapiše vezu **id → objekat**, kao red u tabeli. U moju promenljivu upiše **samo id**, običan ceo broj (npr. 1, 2, 3). Zato prosleđujem adresu promenljive (`&VBO`), da bi drajver imao gde da upiše broj. Prvi parametar (`1`) je koliko objekata pravim.

Kad god kasnije pošaljem taj broj, drajver ga potraži u svojoj tabeli i stigne do pravog objekta. Id je kao broj garderobe: ja imam samo cedulju, a stvar čuva garderober.

Id `0` je rezervisan i znači "nijedan objekat".

### Slotovi i vezivanje

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

Više objekata podesim jednom, na početku. Kasnije pre crtanja samo vežem onaj koji mi treba, bez ponovnog podešavanja. U projektu je to `VAO1.Bind()` u petlji: **VAO je jednom zapamtio koji VBO i EBO koristi i kako čita podatke, pa jedno vezivanje vrati sve to**. O ovim stvarima kasnije, a ovo ispod se može pogledati i kasnije:

> <br>
>
> VAO pamti i VBO i EBO, ali ih **dobija na dva različita načina**. Razlog je u tome gde se nalaze slotovi za ova dva bafera.
>
> ##### Gde su slotovi
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
> - Slot `GL_ARRAY_BUFFER` pripada **kontekstu**.
> - Slot `GL_ELEMENT_ARRAY_BUFFER` pripada **VAO-u**. Kad je VAO vezan, taj slot je polje u njemu.
>
> ##### VBO: prepisuje se u trenutku glVertexAttribPointer
>
> ```cpp
> void VAO::LinkAttrib(VBO& VBO, GLuint layout, GLuint numComponents, GLenum type, GLsizeiptr stride, void* offset)
> {
>     VBO.Bind();
>     glVertexAttribPointer(layout, numComponents, type, GL_FALSE, (GLsizei)stride, offset);
>     glEnableVertexAttribArray(layout);
>     VBO.Unbind();
> }
> ```
>
> `VBO.Bind()` samo upiše id VBO-a u slot `GL_ARRAY_BUFFER` u kontekstu. To samo po sebi ne menja VAO.
>
> VAO saznaje za VBO tek u `glVertexAttribPointer`: funkcija pogleda koji id piše u `GL_ARRAY_BUFFER` i **prepiše ga** u opis atributa u VAO-u ("atribut 0 čita iz ovog VBO-a"). Posle toga `VBO.Unbind()` ne smeta, jer VAO već ima svoju kopiju id-a.
>
> ##### EBO: upisuje se samim vezivanjem
>
> Za EBO ne postoji funkcija kao `LinkAttrib`, jer nije potrebna. Pošto je slot `GL_ELEMENT_ARRAY_BUFFER` deo vezanog VAO-a, vezivanje EBO-a je istovremeno i upisivanje u VAO:
>
> ```cpp
> EBO::EBO(GLuint* indices, GLsizeiptr size)
> {
>     glGenBuffers(1, &ID);
>     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);   // upisuje id EBO-a u vezani VAO
>     glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
> }
> ```
>
> Zato VAO mora biti vezan **pre** nego što se napravi EBO.
>
> ##### Korak po korak u Main.cpp
>
> ```cpp
> VAO VAO1;
> VAO1.Bind();                     // trenutni VAO = VAO1
> VBO VBO1(vertices, sizeof(vertices));
>                                  // GL_ARRAY_BUFFER (kontekst) = VBO1
> EBO EBO1(indices, sizeof(indices));
>                                  // GL_ELEMENT_ARRAY_BUFFER (u VAO1) = EBO1   ← EBO povezan
> VAO1.LinkAttrib(VBO1, 0, 2, GL_FLOAT, 4 * sizeof(float), (void*)0);
>                                  // VAO1.atribut[0].vbo = VBO1               ← VBO povezan
> VAO1.LinkAttrib(VBO1, 1, 2, GL_FLOAT, 4 * sizeof(float), (void*)(2 * sizeof(float)));
>                                  // VAO1.atribut[1].vbo = VBO1
> VAO1.Unbind();                   // trenutni VAO = 0
> VBO1.Unbind();                   // GL_ARRAY_BUFFER = 0, VAO1 ne mari
> EBO1.Unbind();                   // nijedan VAO nije vezan, VAO1 ostaje netaknut
> ```
>
> Redosled na kraju je bitan. Da je `EBO1.Unbind()` pozvan **pre** `VAO1.Unbind()`, upisao bi 0 u slot unutar VAO1. VAO1 bi ostao bez EBO-a, pa `glDrawElements` ne bi imao odakle da čita indekse.
>
> ##### Pri crtanju
>
> ```cpp
> VAO1.Bind();
> glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
> ```
>
> `VAO1.Bind()` vrati sve odjednom: atribute (svaki sa svojim VBO-om) i EBO (jer je njegov slot deo VAO1). Zato jedno vezivanje vrati celu konfiguraciju.
>
> ##### Zašto je napravljeno različito
>
> - **VBO se pamti po atributu**, jer različiti atributi mogu da čitaju iz različitih VBO-ova (npr. pozicije iz jednog bafera, boje iz drugog). Zato svaki atribut ima svoje "iz kog VBO-a", i to se uzima u trenutku `glVertexAttribPointer`.
> - **EBO je jedan za ceo VAO**, jer jedno crtanje koristi jedan niz indeksa. Zato je njegov slot jednostavno deo VAO-a.
>
> ##### Ukratko
>
> - `VBO.Bind()` samo zauzme slot u kontekstu. VAO uzme VBO tek u `glVertexAttribPointer`.
> - `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ...)` dok je VAO vezan odmah upisuje EBO u taj VAO.
> - VAO se razvezuje pre EBO-a, inače ostaje bez EBO-a.
>
> <br>
> 
> 
>
> VAO ne kopira podatke iz VBO-ova. Za svaki atribut pamti samo **uputstvo**: iz kog VBO-a čita, od kog bajta počinje (offset) i koliko bajtova preskače od verteksa do verteksa (stride). Svaki atribut ima svoje uputstvo, pa svaki može čitati iz drugog VBO-a. Pored toga VAO pamti i EBO.
>
> ##### Ponovno vezivanje
>
> Pri ponovnom vezivanju ništa se ne kopira i ništa se ne podešava ponovo. U kontekstu postoji slot "trenutni VAO", a `VAO1.Bind()` (`glBindVertexArray(VAO1.ID)`) samo upiše id VAO1 u taj slot.
>
> Sve što je VAO1 zapamtio (atributi sa svojim VBO-ovima, offset, stride i EBO) i dalje stoji u objektu VAO1 kod drajvera, od trenutka kad je podešen. Funkcije za crtanje uvek gledaju trenutni VAO, pa su sva ta podešavanja od tog trenutka aktivna. Zato izgleda kao da se "sve desi odjednom": promeni se samo jedan broj.
>
> ##### Crtanje
>
> ```cpp
> shaderProgram.Activate();
> VAO1.Bind();
> glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
> ```
>
> Tek u `glDrawElements` se podaci stvarno koriste. Redom:
>
> 1. pogleda trenutni program, tj. koji šejderi se izvršavaju,
> 2. pogleda trenutni VAO (VAO1),
> 3. iz VAO1 uzme EBO i pročita 6 indeksa: `0, 1, 2, 0, 2, 3`,
> 4. za svaki indeks `i` i svaki uključeni atribut izračuna odakle da čita: `VBO tog atributa + offset + i * stride`,
> 5. pročitane vrednosti pošalje u ulaze vertex šejdera sa istim `location` brojem i pokrene ga za taj verteks,
> 6. svaka tri verteksa sklopi u trougao, pa slede rasterizacija i fragment šejder, koji pikselima određuje boju.
>
> ##### Šta je stalno, a šta se menja
>
> VBO i EBO su poslati u memoriju grafičke kartice jednom, pre petlje, i tamo ostaju. Pri svakom crtanju GPU ih samo čita odande, preko jednog VAO-a koji je podešen na početku.
>
> Sam oblik se ne pamti nigde. VBO i EBO sadrže samo kvadrat 1x1, a iz njega se pri svakom crtanju, pomoću uniformi (`uPos`, `uSize`, `uColor`, `uAlpha`) i vezane teksture, iznova dobijaju obojeni pikseli u zadnjem baferu. Posle `glfwSwapBuffers` slika se prikaže, a sledeći frame je `glClear` prefarba i sve se crta ponovo.
>
> - **Stalno u memoriji kartice:** VBO, EBO, VAO, teksture, šejder program.
> - **Menja se pre svakog crtanja:** uniforme i vezana tekstura.
> - **Nastaje svaki frame:** nova slika od piksela.
>
> Zato isti VBO i EBO u istom frame-u daju i pozadinu, i neprovidnu, i poluprovidnu sliku(istu sliku): podaci su isti, razlikuju se samo uniforme i tekstura.
>
> <br>

### Lanac ukratko

1. **Kontekst** je sve što OpenGL pamti.
2. **Objekat** je paket podataka koji drži drajver, a ne moj program.
3. **Id** je broj koji dobijem umesto objekta. Drajver u svojoj tabeli zna koji objekat odgovara kom id-u.
4. **Slot** je mesto u kontekstu u kome piše id trenutno vezanog objekta.
5. **Vezivanje** (`glBind...`) upiše id u slot, a funkcije koje rade preko slota menjaju taj objekat.
6. **Razvezivanje** (`glBind...(..., 0)`) samo isprazni slot. Objekat ostaje, a briše se tek sa `glDelete...`.

### Prozor

#### 1. Inicijalizacija GLFW-a i verzija

```cpp
if (!glfwInit()) {
    std::cout << "Failed to initialize GLFW." << std::endl;
    return -1;
}
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
```

- `glfwInit()` pokreće GLFW, pa tek posle toga može da se podešava.
- `glfwWindowHint` podešava opcije za prozor koji će se tek napraviti, pa mora ići pre `glfwCreateWindow`.
- Traži se OpenGL 3.3. Ako kartica ne podržava tu verziju, `glfwCreateWindow` vraća `NULL`.
- **Core profil** znači samo moderni deo OpenGL-a, bez starih funkcija kao `glBegin`/`glEnd`. To je programabilni pajplajn: crta se isključivo kroz šejdere.

#### 2. Prozor preko celog ekrana i kontekst

```cpp
GLFWmonitor* monitor = glfwGetPrimaryMonitor();
const GLFWvidmode* mode = glfwGetVideoMode(monitor);
GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Perfume Vending Machine", monitor, NULL);
if (window == NULL) return endProgram("Failed to create window.");
glfwMakeContextCurrent(window);
```

- `glfwGetPrimaryMonitor` vraća glavni monitor, a `glfwGetVideoMode` njegov trenutni režim (širinu i visinu).
- Četvrti parametar `glfwCreateWindow` je ono što prozor čini full screen-om. Kad se tu prosledi monitor, prozor zauzima ceo ekran. Sa `NULL` se dobija običan prozor.
- `glfwMakeContextCurrent` postavlja kontekst tog prozora kao aktivan. **Bez aktivnog konteksta nijedna OpenGL funkcija ne radi.**
- `endProgram` ispiše poruku, ugasi GLFW i vrati -1.

#### 3. Učitavanje OpenGL funkcija

```cpp
if (glewInit() != GLEW_OK) return endProgram("Failed to initialize GLEW.");
```

Posle `glfwMakeContextCurrent`, pre bilo koje druge OpenGL funkcije.

#### 4. Viewport

```cpp
glfwGetFramebufferSize(window, &screenW, &screenH);
glViewport(0, 0, screenW, screenH);
```

- `glViewport` kaže OpenGL-u u koji deo prozora crta: donji levi ugao (0, 0), pa širina i visina u pikselima.
- Viewport preslikava koordinate od -1 do 1 u piksele prozora.
- Uzima se veličina **framebuffera**, slike u koju OpenGL zaista crta. Na računarima sa Windows skaliranjem (125%, 150%) ona se može razlikovati od veličine prozora.
- Pošto je prozor full screen i ne menja veličinu, callback za promenu veličine (`glfwSetFramebufferSizeCallback`) nije potreban.

#### 5. Render petlja

```cpp
while (!glfwWindowShouldClose(window))
{
    double frameStart = glfwGetTime();

    glClear(GL_COLOR_BUFFER_BIT);
    // crtanje

    glfwSwapBuffers(window);
    glfwPollEvents();

    while (glfwGetTime() - frameStart < TARGET_FRAME_TIME) {}
}
```

- Jedan prolaz kroz petlju je jedan frame, jedna kompletna slika. Kad se slike brzo smenjuju, oko ih vidi kao kretanje.
- `glfwWindowShouldClose` proverava da li je traženo zatvaranje.
- `glfwPollEvents` obrađuje događaje (tastatura, miš) i poziva registrovane callback-e.
- `glfwSwapBuffers` menja zadnji i prednji bafer.
- Sve što se crta ide između `glClear` i `glfwSwapBuffers`.

#### 6. Dvostruki bafer (double buffering)

- **Prednji bafer** je ono što se trenutno vidi na ekranu.
- **Zadnji bafer** je onaj u koji se crta.
- Kad je crtanje gotovo, baferi se zamene, pa se slika prikazuje tek kad je cela gotova. Time se izbegava treperenje, jer se slika crta deo po deo.

#### 7. Ulaz sa tastature (callback)

```cpp
void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

glfwSetKeyCallback(window, keyboardCallback);
```

- **Callback** je moja funkcija koju GLFW sam pozove kad se desi događaj. Registrujem je jednom preko `glfwSetKeyCallback`, a GLFW je poziva iz `glfwPollEvents()`.
- Potpis mora biti tačno `(GLFWwindow*, int key, int scancode, int action, int mods)`, čak i kad neke parametre ne koristim. Ime funkcije je slobodno.
- `action == GLFW_PRESS` je tu da se reaguje na pritisak, a ne i na puštanje tastera.
- `glfwSetWindowShouldClose` ne gasi program odmah. Samo postavlja zastavicu, a petlja je proveri i završi se, pa program uvek prolazi kroz čišćenje na kraju.

Druga mogućnost je **polling** (`processInput` u knjizi): funkcija koju ja zovem svaki frame i koja pita `glfwGetKey` da li je taster trenutno pritisnut. Callback se izvršava samo kad se događaj zaista desi. U oba slučaja `glfwPollEvents()` mora da se zove svaki frame.

#### 8. Brisanje ekrana

```cpp
glClearColor(0.2f, 0.2f, 0.25f, 1.0f); // menja stanje
glClear(GL_COLOR_BUFFER_BIT);          // koristi stanje
```

- `glClearColor` samo postavlja boju (RGBA), jednom pre petlje.
- `glClear` stvarno briše(prefarba) bafer tom bojom, na početku svakog frame-a, inače bi se videla slika iz prethodnog.

#### 9. V-sync i frame limiter

```cpp
glfwSwapInterval(0);
...
const double TARGET_FPS = 75.0;
const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;
...
while (glfwGetTime() - frameStart < TARGET_FRAME_TIME) {}
```

- **V-sync** tera `glfwSwapBuffers` da čeka osvežavanje monitora, pa bi brzina bila vezana za monitor (60 Hz, 144 Hz...). Projekat traži tačno 75 FPS, pa se v-sync gasi sa `glfwSwapInterval(0)`.
- Na 75 FPS jedan frame sme trajati 1/75 sekunde, oko 13,3 ms. Na početku frame-a zapamti se vreme, a na kraju petlja čeka dok ne prođe 13,3 ms.
- To je aktivno čekanje (busy wait): troši procesor dok čeka, ali je precizno i jednostavno.

#### 10. Čišćenje na kraju

```cpp
glfwDestroyWindow(window);
glfwTerminate();
```

`glfwTerminate` oslobađa sve GLFW resurse. Zato ga `endProgram` poziva i pri grešci. OpenGL objekti se brišu pre toga (vidi Brisanje resursa).

### Grafički pajplajn

Grafički pajplajn pretvara koordinate verteksa u obojene piksele na ekranu, kroz niz koraka, gde svaki korak kao ulaz prima izlaz prethodnog:

1. **Vertex šejder** prima jedan verteks i određuje njegovu poziciju na ekranu.
2. **Sklapanje primitiva** spaja verteksе u zadati oblik (kod nas trouglove, `GL_TRIANGLES`).
3. **Rasterizacija** preslikava trouglove na piksele ekrana i pravi **fragmente**. Fragment su svi podaci potrebni da se iscrta jedan piksel.
4. **Fragment šejder** računa konačnu boju svakog fragmenta.
5. **Blending** meša tu boju sa onim što je već na ekranu, u zavisnosti od alfa vrednosti (providnosti).

Koraci se izvršavaju na grafičkoj kartici, paralelno, na hiljadama malih jezgara. Programi koji se izvršavaju u tim koracima zovu se **šejderi**. U modernom OpenGL-u vertex i fragment šejder moramo da napišemo sami, jer podrazumevani ne postoje.

#### Normalizovane koordinate uređaja (NDC)

Posle vertex šejdera pozicije moraju biti u opsegu od -1 do 1 po x i y. Sve van tog opsega se odseca. (0, 0) je centar ekrana, a y raste nagore, za razliku od koordinata prozora i miša, gde je (0, 0) gornji levi ugao i y raste nadole. Viewport zatim NDC preslikava u piksele.

### Ulaz verteksa

#### Podaci

U celom projektu postoji **samo jedan oblik**: kvadrat veličine 1x1 sa centrom u (0, 0). Mašina, bočice, dugmad i kovanica su isti kvadrat, nacrtan na drugom mestu, u drugoj veličini i sa drugom slikom. Mesto i veličinu mu zadajem uniformama pre svakog crtanja. Zato je dovoljan jedan VBO, jedan EBO i jedan VAO.

```cpp
GLfloat vertices[] = {
    -0.5f, -0.5f,   0.0f, 0.0f,
     0.5f, -0.5f,   1.0f, 0.0f,
     0.5f,  0.5f,   1.0f, 1.0f,
    -0.5f,  0.5f,   0.0f, 1.0f,
};
GLuint indices[] = {
    0, 1, 2,
    0, 2, 3,
};
```

**Verteks** je skup podataka za jednu tačku. Podaci verteksa su podeljeni u **atribute**. Ovde svaki verteks ima dva:

- **poziciju** (x, y): uglovi kvadrata, redom donji levi, donji desni, gornji desni, gornji levi,
- **koordinatu teksture** (u, v): koji ugao slike pripada tom uglu kvadrata.

Projekat je 2D, pa se z ne šalje. Pozicija je `vec2`, a z se u šejderu postavi na 0.

#### VBO (Vertex Buffer Object)

```cpp
VBO::VBO(GLfloat* vertices, GLsizeiptr size)
{
    glGenBuffers(1, &ID);
    glBindBuffer(GL_ARRAY_BUFFER, ID);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}
```

VBO je memorija na grafičkoj kartici u kojoj stoje podaci o verteksima. Slanje podataka sa procesora na karticu je sporo, pa se pošalju jednom i ostanu tamo, a vertex šejder im tamo pristupa gotovo trenutno.

- `glGenBuffers` napravi bafer i u `ID` upiše njegov id.
- `glBindBuffer` veže bafer za slot `GL_ARRAY_BUFFER`.
- `glBufferData` kopira podatke u trenutno vezani bafer. Drugi argument je veličina u bajtovima (`sizeof(vertices)` u `Main.cpp`, što radi jer je `vertices` niz u istoj funkciji, a ne pokazivač).
- Četvrti argument kaže kartici kako će se podaci koristiti:
  - `GL_STREAM_DRAW`: postave se jednom, koriste se nekoliko puta,
  - `GL_STATIC_DRAW`: postave se jednom, koriste se mnogo puta,
  - `GL_DYNAMIC_DRAW`: često se menjaju i koriste mnogo puta.

  Kvadrat se nikad ne menja, a crta se stotine puta po frame-u, pa je `GL_STATIC_DRAW`.

#### EBO (Element Buffer Object)

```cpp
EBO::EBO(GLuint* indices, GLsizeiptr size)
{
    glGenBuffers(1, &ID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
}
```

Kvadrat se crta kao dva trougla. Bez indeksa bi trebalo 6 verteksa, od kojih su dva duplikati. EBO čuva **indekse**, odnosno koji verteksi čine koji trougao, pa su dovoljna 4 verteksa: `0, 1, 2` je prvi trougao, a `0, 2, 3` drugi. To se zove **indeksirano crtanje**. Klasa je ista kao VBO, samo je slot `GL_ELEMENT_ARRAY_BUFFER`.

#### VAO (Vertex Array Object)

```cpp
VAO::VAO()
{
    glGenVertexArrays(1, &ID);
}

void VAO::LinkAttrib(VBO& VBO, GLuint layout, GLuint numComponents, GLenum type, GLsizeiptr stride, void* offset)
{
    VBO.Bind();
    glVertexAttribPointer(layout, numComponents, type, GL_FALSE, (GLsizei)stride, offset);
    glEnableVertexAttribArray(layout);
    VBO.Unbind();
}
```

VAO ne čuva same podatke (to radi VBO), već **pravila** o tome kako se oni čitaju. Za svaki atribut pamti:
- id VBO-a iz kog se čitaju podaci,
- broj komponenti (2 za `vec2`),
- tip podataka (`GL_FLOAT`),
- stride i offset,
- da li je atribut uključen.

Pored toga pamti i EBO vezan za `GL_ELEMENT_ARRAY_BUFFER`. Core OpenGL ne crta ništa bez vezanog VAO-a.

`glVertexAttribPointer` parametri:
- `layout`: koji atribut, odnosno `location` u vertex šejderu,
- `numComponents`: koliko brojeva ima atribut,
- `type`: tip brojeva,
- `GL_FALSE`: brojevi se ne normalizuju, jer su već `float`,
- `stride`: razmak od početka jednog verteksa do početka sledećeg, u bajtovima,
- `offset`: gde unutar verteksa počinje ovaj atribut, u bajtovima. Tipa je `void*`, otuda cast.

`glVertexAttribPointer` ne prima VBO kao parametar. U trenutku poziva pogleda koji je VBO vezan za `GL_ARRAY_BUFFER` i upiše njegov id u trenutno vezani VAO. Zato `LinkAttrib` prvo veže VBO. Kasnija promena vezanja `GL_ARRAY_BUFFER` ne utiče na VAO.

`glEnableVertexAttribArray` uključuje atribut. Atributi su podrazumevano isključeni.

U `Main.cpp`:

```cpp
VAO1.LinkAttrib(VBO1, 0, 2, GL_FLOAT, 4 * sizeof(float), (void*)0);
VAO1.LinkAttrib(VBO1, 1, 2, GL_FLOAT, 4 * sizeof(float), (void*)(2 * sizeof(float)));
```

Jedan verteks u memoriji:

```
bajt:    0       4       8       12      16      20      24      28      32
         ┌───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┐
         │   x   │   y   │   u   │   v   │   x   │   y   │   u   │   v   │
         └───────┴───────┴───────┴───────┴───────┴───────┴───────┴───────┘
         ├──── aPos ─────┤──── aTex ─────┤──── aPos ─────┤──── aTex ─────┤
         ├────────── verteks 0 ──────────┤────────── verteks 1 ──────────┤
```

- Svaki broj je `float`, odnosno 4 bajta, pa jedna ćelija zauzima 4 bajta.
- Jedan verteks ima 4 broja: `4 * 4 = 16` bajtova. To je **stride**, razmak od početka jednog verteksa do početka sljedećeg.
- `aPos` (atribut 0) počinje na bajtu 0 svakog verteksa: **offset 0**.
- `aTex` (atribut 1) počinje posle `x` i `y`, na bajtu 8: **offset 8**, odnosno `2 * sizeof(float)`.

Odakle GPU čita za verteks `i`:

```
aPos:  0 + i * 16   →  bajt 0, 16, 32, 48
aTex:  8 + i * 16   →  bajt 8, 24, 40, 56
```

Upravo to se zadaje u `LinkAttrib`:

```cpp
VAO1.LinkAttrib(VBO1, 0, 2, GL_FLOAT, 4 * sizeof(float), (void*)0);                     // aPos
VAO1.LinkAttrib(VBO1, 1, 2, GL_FLOAT, 4 * sizeof(float), (void*)(2 * sizeof(float)));  // aTex
```

#### Kako VBO komunicira sa šejderom

VBO i vertex šejder nemaju direktnu vezu u kodu. Spaja ih broj lokacije:

- u C++-u `LinkAttrib(VBO1, 0, ...)` opisuje podatke za lokaciju 0,
- u šejderu `layout (location = 0) in vec2 aPos;` prima podatke sa lokacije 0.

Pri crtanju `glDrawElements` uzima atribute iz vezanog VAO-a i šalje ih ulazima šejdera sa istim `location` brojem.

#### Redosled postavljanja

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

1. VAO se veže prvi, jer pamti sve što sledi.
2. VBO se napravi i napuni.
3. EBO se napravi **dok je VAO vezan**, pa ga VAO zapamti. Kasnije `VAO1.Bind()` automatski vrati i EBO.
4. `LinkAttrib` opiše atribute.
5. VAO se razveže **pre** EBO-a. VAO pamti i razvezivanje EBO-a, pa bi u suprotnom ostao bez EBO-a i crtanje ne bi radilo.

Ovo se radi jednom, pre petlje. U petlji je za crtanje dovoljno `VAO1.Bind()`. `VAO VAO1;` mora biti posle `glewInit()`, jer konstruktor već poziva OpenGL funkciju.

### Šejderi

Šejderi su mali programi koji se izvršavaju na grafičkoj kartici. Pišu se u jeziku **GLSL**, koji liči na C. Svaki šejder počinje deklaracijom verzije (`#version 330 core` odgovara OpenGL-u 3.3), zatim slede ulazi, izlazi, uniforme i funkcija `main`.

GLSL ima vektorske tipove `vec2`, `vec3` i `vec4` (2, 3 ili 4 float vrednosti). Komponentama se pristupa preko `.x .y .z .w` ili `.r .g .b .a`, a mogu se i kombinovati (swizzling), npr. `uUVRect.xy` i `uUVRect.zw`.

GLSL ne kompajlira Visual Studio, nego drajver, u toku rada programa. Zato VS ne prijavljuje greške u šejderima, nego ih ispisuje klasa `Shader`.

#### Vertex šejder

```glsl
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTex;

out vec2 texCoord;
out vec2 localPos;

uniform vec2 uScreen;
uniform vec2 uPos;
uniform vec2 uSize;
uniform vec4 uUVRect;

void main()
{
    vec2 pixel = uPos + aPos * uSize;
    vec2 ndc = pixel / uScreen * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);

    texCoord = uUVRect.xy + aTex * uUVRect.zw;
    localPos = aPos;
}
```

Vertex šejder se izvršava jednom za svaki verteks i u `gl_Position` (ugrađena promenljiva tipa `vec4`) upisuje gde je verteks na ekranu.

U `Main.cpp` sve zadajem u pikselima, a šejder ih pretvara u NDC:

1. `uPos + aPos * uSize`: kvadrat 1x1 se razvuče na veličinu `uSize` i pomeri na `uPos`. Za `uSize = (768, 540)` i `uPos = (960, 540)` gornji desni ugao `(0.5, 0.5)` postaje `(1344, 810)` piksela.
2. `pixel / uScreen * 2.0 - 1.0`: pikseli u opseg od -1 do 1. Na ekranu 1920x1080: `1344 / 1920 = 0.7`, pa `0.7 * 2 - 1 = 0.4`.
3. z je 0 jer je scena 2D, a w je 1.

Izlazi za fragment šejder:
- `texCoord`: koji deo slike ide na koji ugao. Sa `uUVRect = (0, 0, 1, 1)` uzima se cela slika, a kasnije se iz atlasa cifara bira samo jedna cifra.
- `localPos`: pozicija unutar kvadrata (od -0.5 do 0.5), koju fragment šejder koristi za krug.

Kada definišem `out` promenljivu, moram joj dodeliti vrednost u `main`, da bi se prosledila dalje.

#### Fragment šejder

```glsl
#version 330 core

in vec2 texCoord;
in vec2 localPos;

out vec4 FragColor;

uniform sampler2D tex0;
uniform int uUseTexture;
uniform int uShape;
uniform vec4 uColor;
uniform vec3 uTint;
uniform float uAlpha;

void main()
{
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
}
```

Fragment šejder se izvršava za svaki fragment i u svoj jedini izlaz (`FragColor`, `vec4`) upisuje konačnu boju. Boja je RGBA, svaka komponenta od 0 do 1, a alfa 1 znači potpuno neprovidno.

- `uUseTexture` bira da li je boja iz teksture ili iz `uColor`.
- `d` je udaljenost od centra kvadrata: 0 u centru, 1 na ivici upisanog kruga.
- `uShape == 1` pravi krug: `smoothstep(0.92, 1.0, d)` vraća 0 unutar 92% poluprečnika, 1 van kruga i glatko prelazi između, pa ivica nije nazubljena.
- `uShape == 2` pravi mekan sjaj koji bledi od centra ka ivici.
- `uAlpha` je providnost celog oblika.
- `discard` odbacuje potpuno providne fragmente, pa se za njih ništa ne upisuje.

#### Veza vertex i fragment šejdera

Fragment šejder se ne povezuje preko VAO-a ni preko `location`, nego preko **imena i tipa**: `out vec2 localPos` u vertex šejderu i `in vec2 localPos` u fragment šejderu su ista promenljiva. To povezivanje radi **linker** pri pravljenju shader programa (`glLinkProgram`). Ako se ime ili tip ne poklapaju, dobija se greška pri linkovanju.

Dva različita spajanja:
1. **VBO → vertex šejder**: preko `location` i VAO-a.
2. **Vertex šejder → fragment šejder**: preko `out`/`in` promenljivih sa istim imenom i tipom.

#### Interpolacija

Vertex šejder se izvrši samo za 4 verteksa, a fragment šejder za svaki piksel unutar kvadrata. Rasterizacija za svaki fragment **interpolira** izlaze vertex šejdera između verteksa. Piksel na pola puta između `localPos = -0.5` i `localPos = 0.5` dobija 0. Isto važi za `texCoord`, zato svaki piksel dobija svoje mesto na slici.

#### Uniforme

Uniforme i atributi služe da se podaci prebace sa procesora na grafičku karticu, ali različito:

- **Atribut** ima drugu vrednost za svaki verteks i dolazi iz VBO-a (pozicija, koordinata teksture). Postoji samo u vertex šejderu.
- **Uniforma** je ista za celo jedno crtanje i postavlja se iz C++-a preko `glUniform...` (pozicija oblika, veličina, boja, providnost). Pripada shader programu i može se koristiti u bilo kom šejderu tog programa, ali mora biti deklarisana u svakom šejderu u kome se koristi.

Uniforma zadržava vrednost dok je ne promenim. Zato se one koje se ne menjaju (`uScreen`, `tex0`) postavljaju jednom pre petlje, a one koje se razlikuju po obliku (`uPos`, `uSize`, `uColor`, `uAlpha`, `uShape`) pre svakog crtanja.

Postfiks `glUniform` funkcije određuje tip: `1i` jedan int, `1f` jedan float, `2f`, `3f`, `4f` vektor od 2, 3 ili 4 floata. OpenGL je C biblioteka i nema preopterećivanje funkcija, pa za svaki tip postoji posebna funkcija.

Ako se uniforma deklariše, a ne koristi, kompajler je izbaci i `glGetUniformLocation` vraća -1. `glUniform` sa lokacijom -1 ne radi ništa i ne javlja grešku.

#### Shader program i klasa Shader

```cpp
Shader::Shader(const char* vertexFile, const char* fragmentFile)
{
    std::string vertexCode = get_file_contents(vertexFile);
    std::string fragmentCode = get_file_contents(fragmentFile);

    const char* vertexSource = vertexCode.c_str();
    const char* fragmentSource = fragmentCode.c_str();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    compileErrors(vertexShader, "VERTEX");

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    compileErrors(fragmentShader, "FRAGMENT");

    ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID);
    compileErrors(ID, "PROGRAM");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}
```

1. `get_file_contents` učita fajl šejdera u string. Ako fajl ne postoji, ispiše koji.
2. `glCreateShader` napravi prazan šejder objekat (`GL_VERTEX_SHADER` ili `GL_FRAGMENT_SHADER`), `glShaderSource` mu da izvorni kod (1 string, `NULL` znači da je string završen nulom), a `glCompileShader` ga kompajlira.
3. `glCreateProgram` napravi **shader program**, konačnu povezanu verziju šejdera. `glAttachShader` mu pridruži šejdere, a `glLinkProgram` ih poveže, spajajući izlaze jednog sa ulazima sledećeg.
4. Pojedinačni šejderi se brišu, jer program ima svoju kopiju.

`compileErrors` za šejder proverava `GL_COMPILE_STATUS` (`glGetShaderiv`, `glGetShaderInfoLog`), a za program `GL_LINK_STATUS` (`glGetProgramiv`, `glGetProgramInfoLog`). Ako nešto nije uspelo, ispiše poruku drajvera. Bez toga bi greška u šejderu značila samo prazan ekran.

```cpp
void Shader::Activate() { glUseProgram(ID); }

void Shader::setVec2(const char* name, float x, float y)
{
    glUniform2f(glGetUniformLocation(ID, name), x, y);
}
```

- `Activate` (`glUseProgram`) bira program kojim se crta.
- `setInt`, `setFloat`, `setVec2`, `setVec3` i `setVec4` pronađu lokaciju uniforme po imenu i postave vrednost.

Pronalaženje lokacije ne traži aktivan program, ali **postavljanje** traži, jer `glUniform...` upisuje u program koji je trenutno aktivan. Zato uvek prvo ide `shaderProgram.Activate()`, pa `set...`.

### Providnost (blending)

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

Bez ovoga se alfa ignoriše. Sa blendingom se nova boja meša sa onom koja je već u baferu:

```
konačna boja = nova boja * alfa + boja ispod * (1 - alfa)
```

Za piksel sa alfom 0.6: 60% nove boje i 40% one ispod. Pošto se meša sa onim što je već nacrtano, **redosled crtanja je bitan**: pozadina se crta prva, a providni objekti posle onoga što treba da se vidi kroz njih. U 2D bez dubinskog bafera ono što je kasnije nacrtano je ispred.

### Crtanje

```cpp
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
```

`glDrawElements` crta koristeći trenutno aktivan šejder program i trenutno vezan VAO, sa indeksima iz EBO-a zapamćenog u tom VAO-u:
- `GL_TRIANGLES`: svaka tri indeksa čine jedan trougao,
- `6`: broj indeksa,
- `GL_UNSIGNED_INT`: tip indeksa, odgovara `GLuint`,
- `0`: offset u EBO-u.

Da se ne bi za svaki oblik pisalo više `set...` poziva, u `Main.cpp` postoje dve pomoćne funkcije:

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

void drawColor(const Rect& r, float red, float green, float blue, float alpha, int shape = 0) {
    setCommonUniforms(r, 1.0f, shape);
    gShader->setInt("uUseTexture", 0);
    gShader->setVec4("uColor", red, green, blue, alpha);
    gShader->setVec4("uUVRect", 0.0f, 0.0f, 1.0f, 1.0f);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
```

`Rect` opisuje pravougaonik na ekranu preko centra (`cx`, `cy`) i veličine (`w`, `h`) u pikselima. `setCommonUniforms` postavlja `uPos`, `uSize`, `uAlpha` i `uShape`. Svaki element na ekranu se tako crta jednim pozivom.

### Teksture

**Tekstura** je 2D slika koja se lepi na oblik. Umesto da se detalji zadaju bojama verteksa, sve je u slici.

#### Koordinate teksture

Svakom verteksu se kaže kom delu slike odgovara, preko **koordinate teksture** (u, v). Koordinate idu od (0, 0) za donji levi ugao slike do (1, 1) za gornji desni. Uzimanje boje iz slike na određenoj koordinati zove se **uzorkovanje** (sampling). Kod nas su to treća i četvrta vrednost svakog verteksa: uglovi kvadrata dobijaju uglove slike, pa se cela slika razvuče preko kvadrata. Rasterizacija interpolira koordinate za svaki fragment.

Pojedinačni piksel teksture zove se **teksel**.

#### Obmotavanje (wrapping)

Šta se dešava sa koordinatama van opsega 0 do 1:
- `GL_REPEAT`: slika se ponavlja (podrazumevano),
- `GL_MIRRORED_REPEAT`: ponavlja se kao u ogledalu,
- `GL_CLAMP_TO_EDGE`: koordinate se ograničavaju na ivicu, pa se razvlači ivični red piksela,
- `GL_CLAMP_TO_BORDER`: van opsega se daje zadata boja ivice.

U projektu je `GL_CLAMP_TO_EDGE` za obe ose (S i T, odnosno x i y). Slike se nikad ne ponavljaju, a sa `GL_REPEAT` bi linearno filtriranje na samoj ivici moglo da pomeša piksele sa suprotne strane slike. To je posebno bitno za atlas cifara, gde je svaka cifra isečak iste slike.

#### Filtriranje

Koordinate teksture ne zavise od rezolucije, pa OpenGL mora da odluči koju boju da da fragmentu koji ne pada tačno na centar teksela:
- `GL_NEAREST`: uzima teksel čiji je centar najbliži (oštri, "pikselizovani" rezultati),
- `GL_LINEAR`: meša susedne teksele po udaljenosti (glatko).

Filtriranje se zadaje posebno za uvećavanje (`GL_TEXTURE_MAG_FILTER`) i umanjivanje (`GL_TEXTURE_MIN_FILTER`).

#### Mipmape

Kad se velika slika crta malo (npr. kovanica od 128x128 piksela nacrtana kao 64x64), jedan fragment pokriva više teksela, pa slika treperi i nazubljena je. **Mipmape** su niz umanjenih kopija slike, svaka duplo manja od prethodne. OpenGL za mali objekat uzima kopiju koja mu odgovara po veličini. `glGenerateMipmap` ih napravi sam.

Za umanjivanje u projektu je `GL_LINEAR_MIPMAP_LINEAR`: linearno između dve najbliže mipmape i linearno unutar njih. Za uvećavanje je `GL_LINEAR`, jer se mipmape koriste samo pri umanjivanju. Mipmap opcija kao filter za uvećavanje daje grešku `GL_INVALID_ENUM`.

#### Učitavanje slike: stb_image

`stb_image.h` je biblioteka u jednom header fajlu koja učitava PNG, JPG i druge formate u niz bajtova. Header sadrži i deklaracije i kod funkcija, a kod se uključuje samo ako je pre include-a definisano `STB_IMAGE_IMPLEMENTATION`. To mora biti u **tačno jednom** `.cpp` fajlu, inače linker javlja da funkcija ne postoji (nigde) ili da je definisana više puta (u dva fajla). U projektu je to `Util.cpp`:

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
```

Svi ostali fajlovi uključuju `stb_image.h` bez `#define` i vide samo deklaracije. Linker njihove pozive poveže sa kodom iz `Util.cpp`.

```cpp
stbi_set_flip_vertically_on_load(true);
unsigned char* bytes = stbi_load(image, &width, &height, &numColCh, 4);
```

- `stbi_load` prima putanju i popunjava širinu, visinu i broj kanala slike. Poslednji argument `4` traži da se svaka slika pretvori u 4 kanala (RGBA), pa je format uvek isti i ne mora da se proverava broj kanala kao na vežbama.
- `stbi_set_flip_vertically_on_load(true)` okreće sliku. OpenGL očekuje da je y = 0 dole, a slike počinju od gornjeg reda, pa bi bez okretanja tekstura bila naopako. Na vežbama je to rađeno sa `stbi__vertical_flip`.

#### Generisanje teksture: klasa Texture

```cpp
glGenTextures(1, &ID);
glActiveTexture(slot);
glBindTexture(texType, ID);

glTexParameteri(texType, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
glTexParameteri(texType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameteri(texType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(texType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

if (bytes)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(texType, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
    glGenerateMipmap(texType);
    stbi_image_free(bytes);
}
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

`glPixelStorei(GL_UNPACK_ALIGNMENT, 1)` kaže da redovi slike nisu poravnati na 4 bajta. Za RGBA uvek jesu, pa je ovo samo zaštita.

Ako slika ne postoji, ispiše se greška i umesto nje se napravi tekstura od jednog ljubičastog piksela, da program ne pukne, a da se odmah vidi šta nedostaje.

#### Primena teksture: sampler i teksturne jedinice

U fragment šejderu tekstura je uniforma tipa **sampler2D**:

```glsl
uniform sampler2D tex0;
...
color = texture(tex0, texCoord) * vec4(uTint, 1.0);
```

`texture` uzorkuje boju slike na interpoliranoj koordinati, sa filtriranjem koje je podešeno. `uTint` množi boju, pa se ista slika može zatamniti ili obojiti (sa (1, 1, 1) ostaje ista).

Vrednost samplera nije sama tekstura, nego broj **teksturne jedinice** iz koje čita. Teksturne jedinice (`GL_TEXTURE0`, `GL_TEXTURE1`...) omogućavaju više tekstura u jednom šejderu. `glActiveTexture` bira jedinicu, a `glBindTexture` veže teksturu za trenutno aktivnu jedinicu:

```cpp
void Texture::Bind()
{
    glActiveTexture(unit);
    glBindTexture(type, ID);
}

void Texture::texUnit(Shader& shader, const char* uniform, GLuint unitIndex)
{
    GLuint texUni = glGetUniformLocation(shader.ID, uniform);
    shader.Activate();
    glUniform1i(texUni, unitIndex);
}
```

U projektu se u jednom crtanju koristi samo jedna slika, pa sve teksture koriste jedinicu 0 (`GL_TEXTURE0`), a `tex0` se jednom, pre petlje, poveže sa jedinicom 0:

```cpp
texBackground.texUnit(shaderProgram, "tex0", 0);
```

Jedinica 0 je podrazumevana, ali je ne postavljaju svi drajveri, pa je bolje postaviti je eksplicitno.

#### Providnost teksture

PNG slike imaju alfa kanal. Oko kovanice su pikseli sa alfom 0, pa se uz blending na tim mestima vidi pozadina i kovanica izgleda okrugla, iako je nacrtana na kvadratu. `uAlpha` se množi sa alfom iz slike (`color.a *= uAlpha`), pa ista slika može biti i neprovidna i poluprovidna:

```cpp
drawTexture(texCoin, Rect{ screenW * 0.4f, screenH / 2.0f, coinSize, coinSize });
drawTexture(texCoin, Rect{ screenW * 0.6f, screenH / 2.0f, coinSize, coinSize }, 0.5f);
```

### Brisanje resursa

```cpp
texBackground.Delete();
texCoin.Delete();

VAO1.Delete();
VBO1.Delete();
EBO1.Delete();
shaderProgram.Delete();

glfwDestroyWindow(window);
glfwTerminate();
```

OpenGL objekti su vezani za kontekst, pa se brišu **pre** `glfwTerminate`, dok kontekst još postoji. Zato klase nemaju destruktor koji poziva `glDelete...`: objekti iz `main` bi bili uništeni tek posle `glfwTerminate`, kad konteksta više nema. Brisanje se radi ručno, metodom `Delete()`.
