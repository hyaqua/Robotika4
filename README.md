# Robotics 4th homework
Lockable box
# Problem
Kai mano jaunesnioji sesė grįžta namo, ji visados eina žaisti kompiuterinius žaidimus ir vengia namų darbų darymą. Ji pradeda daryti namų darbus tik tada, kai mama grįžta iš darbo ir priverčia ją. Norint išsprẹsti šią problemą, aš nusprendžiau paslėpti jos žaidimo kontrolerį dežutėje, kurioje nustatau atrakinimo laiką - 2 valandos po jos grįžimo namo, tačiau ją galima atrakinti anksčiau. Kad tai padarytų, ji turi gauti iš manęs pinkodą arba išspręsti matematinį uždavinį.
# Design
Užrakinamos dežutės dizainas susideda iš dviejų dalių: užrakinamos dežutės ir įvedimo. Užrakinamą dežutę sudaro: servo motoras (atrakinimui ir užrakinimui) ir fotorezistorius (aptikimui šviesos dežutėje). Įvedimui naudojamas lcd ekranas, 2 mygtukai ir 4 potenciometrai. Užrakinant dežutę galima pasirinkti ar dežutė bus užrakinta su pinkodu ar su matematiniu uždaviniu. Matematinis uždavinys gali būti vieno iš trijų sudėtingumo lygių. Uždavinyje naudojami skaitmenys ir atsakymas yra automatiškai sugeneruojami.


Components:
|Quantity|Component             |
|--------|----------------------|
|2       | Pushbutton           |
|1       | LCD 16 x 2           |
|1       |Positional Micro Servo|
|1       | Photoresistor        |
|4       |50 kΩ Potentiometer   |
|2       |10 kΩ Resistor        |
|1       |1 kΩ Resistor         |
|1       |220 Ω Resistor        |

# Design and schematics

## Design
<img width="1925" height="1042" alt="Mighty Amur" src="https://raw.githubusercontent.com/hyaqua/Robotika4/refs/heads/main/assets/tinkercad.png" />

## Schematics
<img width="2592" height="2003" alt="Screenshot 2025-11-04 at 11-29-51 Circuit design Mighty Amur - Tinkercad" src="https://raw.githubusercontent.com/hyaqua/Robotika4/refs/heads/main/assets/schematic.png" />

# Demo video
To be added

# Encountered problems
- Servo has only a range 150 degrees, where I thought it would have 180, which messed up some of my earlier design
- Photoresistors not giving real values, the grounding on them was incorrect at first which caused random values to be sent to the arduino instead of the real ones
- PWM, servo library and timer interrupt conflicts, because PWM uses different timers for different pins, and the default Servo library uses Timer1 for it's calculations, i had to use a different servo library that utilizes Timer2.
# Future improvements
- Get a stronger servo
- Add an alarm
