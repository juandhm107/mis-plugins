#pragma once

#include <vector>
#include <utility>

// Sonidos de fabrica de Dembow.
// Cada preset parte del sonido "Inicial" (los valores por defecto)
// y cambia solo lo que se indica aqui, en unidades reales
// (segundos, Hz, indices de las listas).
//
// Listas:
//   onda:   0 Seno, 1 Triangulo, 2 Sierra, 3 Cuadrada, 4 Pulso, 5 Super sierra, 6 Organo
//   filtro: 0 Grave 24, 1 Grave 12, 2 Banda, 3 Agudo
//   lfo:    0 Seno, 1 Triangulo, 2 Cuadrada, 3 Aleatorio
//   destino LFO: 0 Tono, 1 Filtro, 2 Volumen, 3 Ancho de pulso
//   modo:   0 Poli, 1 Mono, 2 Legato
//   delay:  0 1/4, 1 1/8 punteado, 2 1/8, 3 1/16, 4 1/4 tresillo, 5 1/8 tresillo
namespace dembow
{
    enum Category { Plucks = 0, Bajos, Leads, Brass, Pads, Teclados, NumCategories };

    inline const char* categoryName (int c)
    {
        static const char* names[] = { "PLUCKS", "BAJOS", "LEADS", "BRASS", "PADS", "TECLADOS" };
        return names[c];
    }

    struct PresetDef
    {
        const char* name;
        int category;
        bool oldSchool;
        const char* info;
        std::vector<std::pair<const char*, float>> values;
    };

    inline const std::vector<PresetDef>& presets()
    {
        static const std::vector<PresetDef> list =
        {
            //================================================================ PLUCKS
            { "Pluck Perreo Viejo", Plucks, true, "Sierras desafinadas con filtro que se cierra rapido, sonido 2004",
              { {"o1wave",2}, {"o2wave",3}, {"o2level",0.55f}, {"o2fine",9}, {"o1fine",-6},
                {"cutoff",380}, {"reso",0.22f}, {"fenv",0.72f}, {"fa",0.001f}, {"fd",0.24f}, {"fs",0},
                {"aa",0.001f}, {"ad",0.5f}, {"as",0}, {"ar",0.28f},
                {"chorus",0.3f}, {"dlymix",0.2f}, {"dlytime",1}, {"dlyfb",0.3f}, {"revmix",0.15f}, {"volume",-1.8f} } },

            { "Marimba Perreo", Plucks, true, "Marimba FM, clasica del reggaeton de los 2000",
              { {"o1wave",0}, {"o2wave",0}, {"o2oct",2}, {"o2level",0}, {"fm",0.04f}, {"fmenv",0.42f},
                {"cutoff",14000}, {"keytrk",0}, {"fa",0.001f}, {"fd",0.06f}, {"fs",0},
                {"aa",0.001f}, {"ad",0.55f}, {"as",0}, {"ar",0.3f}, {"velsens",0.6f}, {"revmix",0.15f}, {"volume",-6.3f} } },

            { "Steel Drum Clasico", Plucks, true, "Tambor metalico de isla, muy old school",
              { {"o1wave",0}, {"o2wave",0}, {"o2oct",1}, {"o2semi",7}, {"o2level",0}, {"fm",0.1f}, {"fmenv",0.28f},
                {"sub",0.15f}, {"cutoff",12000}, {"keytrk",0}, {"fa",0.001f}, {"fd",0.22f}, {"fs",0},
                {"aa",0.001f}, {"ad",0.7f}, {"as",0}, {"ar",0.35f}, {"revmix",0.18f}, {"volume",-7.0f} } },

            { "Pluck Suave Moderno", Plucks, false, "Pluck redondo y suave, para melodias romanticas",
              { {"o1wave",1}, {"o2wave",0}, {"o2oct",1}, {"o2level",0.35f},
                {"ftype",1}, {"cutoff",900}, {"fenv",0.45f}, {"fa",0.001f}, {"fd",0.35f}, {"fs",0.05f},
                {"aa",0.002f}, {"ad",0.65f}, {"as",0}, {"ar",0.4f},
                {"dlymix",0.24f}, {"dlytime",1}, {"dlyfb",0.35f}, {"revmix",0.3f}, {"revsize",0.65f}, {"volume",-6.0f} } },

            { "Pluck Cristal", Plucks, false, "Campanita brillante con eco, tipo moderno",
              { {"o1wave",0}, {"o2wave",0}, {"o2oct",1}, {"o2semi",7}, {"o2level",0}, {"fm",0.05f}, {"fmenv",0.32f},
                {"cutoff",16000}, {"keytrk",0}, {"fa",0.001f}, {"fd",0.35f}, {"fs",0},
                {"aa",0.001f}, {"ad",0.95f}, {"as",0}, {"ar",0.7f},
                {"dlymix",0.22f}, {"dlytime",1}, {"dlyfb",0.4f}, {"revmix",0.35f}, {"revsize",0.7f}, {"volume",-7.5f} } },

            { "Kalimba", Plucks, false, "Kalimba dulce, perfecta para intros",
              { {"o1wave",0}, {"o2wave",0}, {"o2oct",2}, {"o2semi",5}, {"o2level",0}, {"fm",0.02f}, {"fmenv",0.3f},
                {"cutoff",12000}, {"keytrk",0}, {"fa",0.001f}, {"fd",0.04f}, {"fs",0},
                {"aa",0.001f}, {"ad",0.7f}, {"as",0}, {"ar",0.4f}, {"revmix",0.25f}, {"volume",-6.8f} } },

            { "Guitarra Pluck", Plucks, false, "Pluck tipo guitarra sintetica con chorus",
              { {"o1wave",4}, {"pw",0.28f}, {"o2wave",2}, {"o2level",0.4f}, {"o2fine",7},
                {"cutoff",650}, {"reso",0.2f}, {"fenv",0.55f}, {"fa",0.001f}, {"fd",0.3f}, {"fs",0.05f},
                {"aa",0.001f}, {"ad",1.0f}, {"as",0}, {"ar",0.4f},
                {"chorus",0.4f}, {"revmix",0.2f}, {"volume",-8.0f} } },

            { "Pluck Supersaw", Plucks, false, "Pluck gordo de 7 sierras para drops",
              { {"o1wave",5}, {"detune",0.32f}, {"cutoff",480}, {"fenv",0.7f}, {"fa",0.001f}, {"fd",0.28f}, {"fs",0},
                {"aa",0.001f}, {"ad",0.42f}, {"as",0}, {"ar",0.3f},
                {"dlymix",0.18f}, {"dlytime",3}, {"dlyfb",0.3f}, {"revmix",0.25f}, {"volume",3.4f} } },

            //================================================================ BAJOS
            { "808 Moderno", Bajos, false, "808 largo con un poco de saturacion y glide",
              { {"o1wave",0}, {"o1level",1.0f}, {"cutoff",2500}, {"keytrk",0}, {"drive",0.35f},
                {"aa",0.002f}, {"ad",1.6f}, {"as",0.55f}, {"ar",0.35f},
                {"mode",2}, {"glide",0.08f}, {"velsens",0.2f}, {"volume",0.4f} } },

            { "808 Sucio", Bajos, false, "808 distorsionado, pega duro en el trap-reggaeton",
              { {"o1wave",0}, {"o2wave",1}, {"o2level",0.3f}, {"o1level",1.0f}, {"cutoff",1400}, {"reso",0.1f},
                {"keytrk",0}, {"drive",0.9f}, {"aa",0.002f}, {"ad",2.0f}, {"as",0.4f}, {"ar",0.3f},
                {"mode",2}, {"glide",0.12f}, {"velsens",0.15f}, {"volume",2.7f} } },

            { "Bajo Dembow Clasico", Bajos, true, "Bajo cuadrado con sub, el de toda la vida",
              { {"o1wave",3}, {"o1level",0.6f}, {"sub",0.8f}, {"cutoff",420}, {"fenv",0.35f},
                {"fa",0.001f}, {"fd",0.25f}, {"fs",0.2f}, {"keytrk",0.3f},
                {"aa",0.002f}, {"ad",0.5f}, {"as",0.7f}, {"ar",0.1f}, {"mode",1}, {"velsens",0.3f}, {"volume",0.0f} } },

            { "Sub Bajo", Bajos, false, "Seno puro y limpio, solo graves",
              { {"o1wave",0}, {"o1level",1.0f}, {"cutoff",1200}, {"keytrk",0},
                {"aa",0.003f}, {"ad",0.5f}, {"as",1.0f}, {"ar",0.15f}, {"mode",2}, {"glide",0.03f}, {"velsens",0.1f}, {"volume",-0.8f} } },

            { "Bajo Synth Old School", Bajos, true, "Sierra filtrada con resonancia, tipo 2005",
              { {"o1wave",2}, {"o1level",0.8f}, {"sub",0.5f}, {"cutoff",300}, {"reso",0.38f}, {"fenv",0.5f},
                {"fa",0.001f}, {"fd",0.2f}, {"fs",0.2f}, {"keytrk",0.3f},
                {"aa",0.002f}, {"ad",0.4f}, {"as",0.75f}, {"ar",0.1f}, {"mode",2}, {"glide",0.03f}, {"volume",2.9f} } },

            { "Reese Moderno", Bajos, false, "Dos sierras muy desafinadas, bajo oscuro que se mueve",
              { {"o1wave",2}, {"o1fine",-14}, {"o2wave",2}, {"o2fine",14}, {"o2level",0.8f}, {"sub",0.5f},
                {"cutoff",650}, {"reso",0.15f}, {"keytrk",0.2f}, {"drive",0.25f},
                {"lfowave",0}, {"lfodest",1}, {"lforate",0.3f}, {"lfodepth",0.25f},
                {"aa",0.005f}, {"ad",0.5f}, {"as",0.9f}, {"ar",0.15f}, {"mode",2}, {"glide",0.05f}, {"volume",0.6f} } },

            { "Bajo Pluck", Bajos, true, "Bajo corto y seco para dembow rapido",
              { {"o1wave",2}, {"o2wave",3}, {"o2oct",-1}, {"o2level",0.5f}, {"cutoff",260}, {"fenv",0.6f},
                {"fa",0.001f}, {"fd",0.15f}, {"fs",0}, {"aa",0.001f}, {"ad",0.35f}, {"as",0}, {"ar",0.12f},
                {"mode",1}, {"volume",5.5f} } },

            { "Bajo Funk", Bajos, true, "Bajo gordo con barrido de filtro",
              { {"o1wave",2}, {"o2wave",4}, {"pw",0.35f}, {"o2level",0.6f}, {"cutoff",320}, {"reso",0.5f},
                {"fenv",0.55f}, {"fa",0.001f}, {"fd",0.3f}, {"fs",0.1f}, {"keytrk",0.3f},
                {"aa",0.002f}, {"ad",0.5f}, {"as",0.8f}, {"ar",0.1f}, {"mode",2}, {"glide",0.05f}, {"volume",6.0f} } },

            //================================================================ LEADS
            { "Lead Clasico 2004", Leads, true, "Lead cuadrado y brillante del reggaeton de antes",
              { {"o1wave",3}, {"o2wave",2}, {"o2level",0.5f}, {"o2fine",8}, {"cutoff",3200}, {"fenv",0.2f},
                {"fa",0.001f}, {"fd",0.3f}, {"fs",0.4f},
                {"lfowave",0}, {"lfodest",0}, {"lforate",5.5f}, {"lfodepth",0.12f}, {"lfofade",0.35f},
                {"aa",0.004f}, {"ad",0.3f}, {"as",0.85f}, {"ar",0.15f}, {"mode",2}, {"glide",0.04f},
                {"dlymix",0.15f}, {"dlytime",1}, {"revmix",0.15f}, {"volume",-6.8f} } },

            { "Lead Silbido", Leads, true, "Silbido sintetico con vibrato",
              { {"o1wave",0}, {"o2wave",1}, {"o2oct",1}, {"o2level",0.2f}, {"noise",0.03f}, {"cutoff",6000},
                {"lfowave",0}, {"lfodest",0}, {"lforate",5.8f}, {"lfodepth",0.16f}, {"lfofade",0.4f},
                {"aa",0.03f}, {"ad",0.3f}, {"as",0.9f}, {"ar",0.25f}, {"mode",2}, {"glide",0.1f},
                {"dlymix",0.2f}, {"dlytime",1}, {"revmix",0.3f}, {"volume",-5.5f} } },

            { "Lead Cuadrado", Leads, true, "Pulso nasal tipo videojuego, muy de la epoca",
              { {"o1wave",4}, {"pw",0.25f}, {"ftype",1}, {"cutoff",5000},
                {"aa",0.002f}, {"ad",0.2f}, {"as",0.8f}, {"ar",0.1f}, {"mode",1}, {"glide",0.03f},
                {"chorus",0.2f}, {"revmix",0.12f}, {"volume",-3.5f} } },

            { "Lead Flauta Sintetica", Leads, true, "Flauta de sintetizador con soplido",
              { {"o1wave",1}, {"o2wave",0}, {"o2oct",1}, {"o2level",0.25f}, {"noise",0.05f}, {"ftype",1}, {"cutoff",3500},
                {"lfowave",0}, {"lfodest",0}, {"lforate",5.0f}, {"lfodepth",0.13f}, {"lfofade",0.5f},
                {"aa",0.05f}, {"ad",0.3f}, {"as",0.85f}, {"ar",0.25f}, {"mode",2}, {"glide",0.05f}, {"revmix",0.25f}, {"volume",-3.5f} } },

            { "Lead Sirena", Leads, true, "Sirena que sube y baja, para efectos e intros",
              { {"o1wave",2}, {"o2wave",3}, {"o2level",0.5f}, {"cutoff",4000},
                {"lfowave",1}, {"lfodest",0}, {"lforate",0.45f}, {"lfodepth",0.55f},
                {"aa",0.05f}, {"ad",0.3f}, {"as",1.0f}, {"ar",0.4f}, {"mode",1}, {"glide",0.3f}, {"revmix",0.25f}, {"volume",4.4f} } },

            { "Lead Supersaw", Leads, false, "Lead ancho de 7 sierras, moderno y grande",
              { {"o1wave",5}, {"detune",0.42f}, {"cutoff",6500}, {"fenv",0.1f},
                {"aa",0.005f}, {"ad",0.4f}, {"as",0.85f}, {"ar",0.3f},
                {"dlymix",0.2f}, {"dlytime",1}, {"revmix",0.35f}, {"volume",-3.7f} } },

            { "Lead Acid Perreo", Leads, false, "Sierra resonante con glide, chillona",
              { {"o1wave",2}, {"cutoff",600}, {"reso",0.72f}, {"fenv",0.6f}, {"fa",0.001f}, {"fd",0.22f}, {"fs",0.2f},
                {"drive",0.4f}, {"aa",0.002f}, {"ad",0.3f}, {"as",0.8f}, {"ar",0.12f}, {"mode",2}, {"glide",0.07f},
                {"dlymix",0.15f}, {"dlytime",3}, {"volume",2.6f} } },

            { "Lead Moderno Brillante", Leads, false, "Lead limpio con eco para melodias modernas",
              { {"o1wave",4}, {"pw",0.4f}, {"o2wave",2}, {"o2oct",1}, {"o2level",0.35f}, {"cutoff",4200}, {"fenv",0.15f},
                {"lfowave",0}, {"lfodest",0}, {"lforate",5.0f}, {"lfodepth",0.1f}, {"lfofade",0.5f},
                {"aa",0.003f}, {"ad",0.3f}, {"as",0.8f}, {"ar",0.3f}, {"mode",2}, {"glide",0.05f},
                {"chorus",0.3f}, {"dlymix",0.25f}, {"dlytime",1}, {"revmix",0.3f}, {"volume",-4.1f} } },

            //================================================================ BRASS
            { "Brass Perreo Viejo", Brass, true, "Metales sinteticos clasicos del reggaeton de los 2000",
              { {"o1wave",2}, {"o1fine",-10}, {"o2wave",2}, {"o2fine",10}, {"o2level",0.8f},
                {"cutoff",850}, {"fenv",0.45f}, {"fa",0.05f}, {"fd",0.35f}, {"fs",0.4f},
                {"aa",0.02f}, {"ad",0.3f}, {"as",0.8f}, {"ar",0.15f}, {"chorus",0.25f}, {"revmix",0.2f}, {"volume",-8.1f} } },

            { "Stab Metales", Brass, true, "Golpe corto de metales para los acordes",
              { {"o1wave",2}, {"o2wave",3}, {"o2fine",12}, {"o2level",0.6f}, {"cutoff",1100}, {"fenv",0.5f},
                {"fa",0.001f}, {"fd",0.18f}, {"fs",0.1f}, {"aa",0.002f}, {"ad",0.3f}, {"as",0}, {"ar",0.12f},
                {"revmix",0.25f}, {"volume",-1.5f} } },

            { "Hit Orquesta", Brass, true, "El golpe de orquesta de los 90 y 2000",
              { {"o1wave",2}, {"o2wave",3}, {"o2oct",-1}, {"o2level",0.7f}, {"noise",0.3f}, {"sub",0.3f},
                {"cutoff",2400}, {"fenv",0.4f}, {"fa",0.001f}, {"fd",0.15f}, {"fs",0.2f},
                {"aa",0.001f}, {"ad",0.45f}, {"as",0}, {"ar",0.4f}, {"revmix",0.4f}, {"revsize",0.7f}, {"volume",-9.5f} } },

            { "Trompetas Synth", Brass, true, "Trompetas con vibrato que entra despacio",
              { {"o1wave",2}, {"o2wave",2}, {"o2oct",1}, {"o2level",0.3f}, {"cutoff",900}, {"fenv",0.5f},
                {"fa",0.03f}, {"fd",0.4f}, {"fs",0.6f},
                {"lfowave",0}, {"lfodest",0}, {"lforate",5.5f}, {"lfodepth",0.1f}, {"lfofade",0.4f},
                {"aa",0.03f}, {"ad",0.3f}, {"as",0.85f}, {"ar",0.2f}, {"revmix",0.2f}, {"volume",-8.0f} } },

            { "Brass 80s", Brass, false, "Brass con ataque lento, ochentoso y moderno a la vez",
              { {"o1wave",2}, {"o2wave",4}, {"pw",0.35f}, {"o2level",0.6f}, {"o2fine",6},
                {"cutoff",700}, {"fenv",0.5f}, {"fa",0.08f}, {"fd",0.5f}, {"fs",0.5f},
                {"aa",0.03f}, {"ad",0.5f}, {"as",0.8f}, {"ar",0.3f}, {"chorus",0.35f}, {"revmix",0.25f}, {"volume",-8.9f} } },

            { "Brass Supersaw", Brass, false, "Metales grandes de 7 sierras",
              { {"o1wave",5}, {"detune",0.25f}, {"cutoff",1500}, {"fenv",0.45f}, {"fa",0.04f}, {"fd",0.4f}, {"fs",0.5f},
                {"aa",0.02f}, {"ad",0.4f}, {"as",0.8f}, {"ar",0.25f}, {"revmix",0.3f}, {"volume",-3.6f} } },

            { "Brass Suave", Brass, false, "Metales tranquilos para fondos",
              { {"o1wave",2}, {"o2wave",1}, {"o2level",0.5f}, {"ftype",1}, {"cutoff",800}, {"fenv",0.35f},
                {"fa",0.1f}, {"fd",0.6f}, {"fs",0.6f}, {"aa",0.08f}, {"ad",0.5f}, {"as",0.85f}, {"ar",0.4f},
                {"chorus",0.3f}, {"revmix",0.3f}, {"volume",-6.5f} } },

            //================================================================ PADS
            { "Pad Romantico", Pads, true, "Colchon calido para el reggaeton romantico",
              { {"o1wave",5}, {"detune",0.3f}, {"o2wave",1}, {"o2oct",-1}, {"o2level",0.4f}, {"cutoff",2400},
                {"aa",0.6f}, {"ad",1.0f}, {"as",0.8f}, {"ar",1.5f}, {"chorus",0.4f}, {"revmix",0.5f}, {"revsize",0.75f}, {"volume",-4.5f} } },

            { "Cuerdas Old School", Pads, true, "Cuerdas sinteticas de los 2000",
              { {"o1wave",2}, {"o1fine",-8}, {"o2wave",2}, {"o2fine",8}, {"o2level",0.8f}, {"cutoff",3500},
                {"lfowave",0}, {"lfodest",0}, {"lforate",4.5f}, {"lfodepth",0.07f},
                {"aa",0.3f}, {"ad",0.8f}, {"as",0.85f}, {"ar",0.8f}, {"chorus",0.5f}, {"revmix",0.4f}, {"volume",-7.5f} } },

            { "Pad Atmosfera", Pads, false, "Pad que respira solo, para intros modernas",
              { {"o1wave",2}, {"o2wave",2}, {"o2semi",7}, {"o2level",0.5f}, {"cutoff",1500}, {"reso",0.2f},
                {"lfowave",0}, {"lfodest",1}, {"lforate",0.15f}, {"lfodepth",0.35f},
                {"aa",1.2f}, {"ad",1.0f}, {"as",0.9f}, {"ar",2.0f},
                {"dlymix",0.2f}, {"dlytime",0}, {"dlyfb",0.4f}, {"revmix",0.6f}, {"revsize",0.85f}, {"volume",-9.7f} } },

            { "Pad Oscuro", Pads, false, "Pad grave y misterioso",
              { {"o1wave",4}, {"pw",0.3f}, {"o2wave",2}, {"o2oct",-1}, {"o2level",0.5f}, {"cutoff",850},
                {"lfowave",1}, {"lfodest",3}, {"lforate",0.3f}, {"lfodepth",0.6f},
                {"aa",0.8f}, {"ad",1.0f}, {"as",0.85f}, {"ar",2.0f}, {"revmix",0.5f}, {"revsize",0.8f}, {"volume",-13.4f} } },

            { "Pad Coro", Pads, true, "Voces sinteticas tipo coro",
              { {"o1wave",1}, {"o2wave",4}, {"pw",0.3f}, {"o2level",0.5f}, {"ftype",2}, {"cutoff",1100}, {"reso",0.4f},
                {"keytrk",0.3f}, {"lfowave",0}, {"lfodest",1}, {"lforate",0.5f}, {"lfodepth",0.12f},
                {"aa",0.4f}, {"ad",0.8f}, {"as",0.9f}, {"ar",1.0f}, {"chorus",0.5f}, {"revmix",0.5f}, {"volume",1.9f} } },

            { "Pad Cristalino", Pads, false, "Pad brillante con campanitas",
              { {"o1wave",0}, {"o2wave",0}, {"o2oct",2}, {"o2level",0.15f}, {"fm",0.08f}, {"fmenv",0.1f},
                {"sub",0.2f}, {"cutoff",12000}, {"keytrk",0}, {"fa",0.5f}, {"fd",2.0f}, {"fs",0.3f},
                {"aa",0.5f}, {"ad",1.0f}, {"as",0.8f}, {"ar",2.5f},
                {"dlymix",0.3f}, {"dlytime",1}, {"dlyfb",0.45f}, {"revmix",0.7f}, {"revsize",0.9f}, {"volume",-11.0f} } },

            { "Pad Barrido", Pads, false, "Supersaw que se abre poco a poco",
              { {"o1wave",5}, {"detune",0.35f}, {"cutoff",300}, {"reso",0.3f}, {"fenv",0.6f},
                {"fa",2.5f}, {"fd",2.0f}, {"fs",0.6f}, {"fr",1.5f},
                {"aa",0.4f}, {"ad",1.0f}, {"as",0.9f}, {"ar",1.5f}, {"revmix",0.5f}, {"revsize",0.8f}, {"volume",-3.7f} } },

            //================================================================ TECLADOS
            { "Piano Electrico", Teclados, true, "Piano electrico FM con tremolo",
              { {"o1wave",0}, {"o2wave",0}, {"o2level",0}, {"fm",0.07f}, {"fmenv",0.3f},
                {"cutoff",14000}, {"keytrk",0}, {"fa",0.001f}, {"fd",1.2f}, {"fs",0},
                {"lfowave",0}, {"lfodest",2}, {"lforate",4.5f}, {"lfodepth",0.15f},
                {"aa",0.002f}, {"ad",2.5f}, {"as",0.15f}, {"ar",0.4f}, {"velsens",0.7f},
                {"chorus",0.3f}, {"revmix",0.2f}, {"volume",-4.6f} } },

            { "Rhodes Moderno", Teclados, false, "Rhodes suave para acordes R&B y reggaeton lento",
              { {"o1wave",0}, {"o2wave",0}, {"o2level",0}, {"fm",0.04f}, {"fmenv",0.2f}, {"sub",0.1f},
                {"ftype",1}, {"cutoff",4500}, {"keytrk",0}, {"fa",0.001f}, {"fd",1.5f}, {"fs",0},
                {"lfowave",0}, {"lfodest",2}, {"lforate",5.0f}, {"lfodepth",0.1f},
                {"aa",0.002f}, {"ad",3.0f}, {"as",0.15f}, {"ar",0.5f}, {"velsens",0.6f},
                {"chorus",0.35f}, {"revmix",0.3f}, {"volume",-4.4f} } },

            { "Organo Old School", Teclados, true, "Organo con vibrato, de los primeros perreos",
              { {"o1wave",6}, {"cutoff",9000}, {"keytrk",0},
                {"lfowave",0}, {"lfodest",0}, {"lforate",6.2f}, {"lfodepth",0.08f},
                {"aa",0.005f}, {"ad",0.1f}, {"as",1.0f}, {"ar",0.06f}, {"velsens",0.1f},
                {"chorus",0.4f}, {"revmix",0.15f}, {"volume",-3.4f} } },

            { "Organo Moderno", Teclados, false, "Organo oscuro y limpio",
              { {"o1wave",6}, {"ftype",1}, {"cutoff",2500}, {"keytrk",0.3f},
                {"aa",0.005f}, {"ad",0.1f}, {"as",1.0f}, {"ar",0.15f}, {"velsens",0.2f},
                {"chorus",0.2f}, {"revmix",0.3f}, {"volume",-4.1f} } },

            { "Piano Sintetico", Teclados, true, "Piano de teclado barato de los 2000",
              { {"o1wave",1}, {"o2wave",4}, {"pw",0.3f}, {"o2level",0.3f}, {"cutoff",2200}, {"fenv",0.4f},
                {"fa",0.001f}, {"fd",0.6f}, {"fs",0.2f},
                {"aa",0.001f}, {"ad",2.0f}, {"as",0.1f}, {"ar",0.4f}, {"velsens",0.6f}, {"revmix",0.2f}, {"volume",-5.8f} } },

            { "Clavinet", Teclados, true, "Teclado funky y percusivo",
              { {"o1wave",4}, {"pw",0.15f}, {"cutoff",1500}, {"reso",0.4f}, {"fenv",0.5f},
                {"fa",0.001f}, {"fd",0.2f}, {"fs",0.1f},
                {"aa",0.001f}, {"ad",0.8f}, {"as",0.3f}, {"ar",0.1f}, {"velsens",0.6f}, {"volume",-5.5f} } },

            { "Campanas Old School", Teclados, true, "Campanas FM que suenan en tantos clasicos",
              { {"o1wave",0}, {"o2wave",0}, {"o2oct",1}, {"o2semi",10}, {"o2fine",-31}, {"o2level",0},
                {"fm",0.1f}, {"fmenv",0.35f}, {"cutoff",14000}, {"keytrk",0}, {"fa",0.001f}, {"fd",1.5f}, {"fs",0},
                {"aa",0.001f}, {"ad",3.0f}, {"as",0}, {"ar",1.5f},
                {"dlymix",0.15f}, {"dlytime",1}, {"revmix",0.35f}, {"volume",-8.3f} } },

            { "Celesta Perreo", Teclados, false, "Celesta dulce y brillante",
              { {"o1wave",0}, {"o2wave",0}, {"o2oct",2}, {"o2level",0.15f}, {"fm",0.03f}, {"fmenv",0.3f},
                {"cutoff",14000}, {"keytrk",0}, {"fa",0.001f}, {"fd",0.4f}, {"fs",0},
                {"aa",0.001f}, {"ad",1.5f}, {"as",0}, {"ar",0.8f}, {"revmix",0.3f}, {"volume",-8.0f} } },
        };
        return list;
    }
}
