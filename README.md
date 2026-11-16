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
