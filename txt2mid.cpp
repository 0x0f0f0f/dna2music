
// TXT2MID

// --------------------------
// convertisseur .txt => .mid
// --------------------------

// version 2.0 (novembre 2016)
// par Jamil Alioui

/* syntaxe du fichier texte :

   pour une note :
   ---------------

        N <esp> date <esp> octave <esp> note <esp> vélocité <esp> durée <esp> track

        0 <= note <= 11
        0 <= vélocité <= 127
        le n° de track commence à 1 et non à 0
        la date et la durée de la note utilisent la règle noire = 240

   pour un contrôleur :
   --------------------

        C <esp> date <esp> cc <esp> valeur <esp> track

        0 <= cc <= 127
        0 <= valeur <= 127
        mêmes autres réglages que pour les notes

   pour la molette de pitch :
   --------------------------

        P <esp> date <esp> valeur <esp> track

        0 <= valeur <= 16383 (médiane = 8192)

   pour une fréquence :
   --------------------

        H <esp> date <esp> fréquence <esp> vélocité <esp> durée <esp> track

        30 <= fréquence <= 8346
        mêmes autres réglages que pour les notes
        (pitch bend range = 200 cents)

   pour un commentaire :
   ---------------------

        _ commentaire bla bla bla _

   "hacks" : (doivent apparaître en début de fichier txt)
   ---------

        # ja_spectral
        Permet l'utilisation de la notation spectrale conçue par Jamil Alioui

 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <cmath>
#include <float.h>

using namespace std;

// structure des données

typedef struct lineData {
    char type; // type dans le texte
    char miditype; // type dans le midi
    int start; // date de début ou de changement de cc
    int octave; // n° d'octave
    int hauteur; // hauteur de note (0 = do, 1 = do#, etc.)
    int velocite; // vélocité de note (entre 0 et 127 compris)
    int duree; // durée de note
    int controleur; // n° de contrôleur
    int valeur; // valeur de contrôleur
    int track; // piste (commence à 1)
} lineData;

// fréquences des notes
float freqs[97] = {
    /* octave 0 */
    32.70f, 34.65f, 36.71f, 38.89f, 41.20f, 43.65f, 46.25f, 49.00f, 51.91f, 55.00f, 58.27f, 61.74f,
    /* octave 1 */
    65.41f, 69.30f, 73.42f, 77.78f, 82.41f, 87.31f, 92.50f, 98.00f, 103.83f, 110.00f, 116.54f, 123.47f,
    /* octave 2 */
    130.81f, 138.59f, 146.83f, 155.56f, 164.81f, 174.61f, 185.00f, 196.00f, 207.65f, 220.00f, 233.08f, 246.94f,
    /* octave 3 */
    261.63f, 277.18f, 293.66f, 311.13f, 329.63f, 349.23f, 369.99f, 392.00f, 415.30f, 440.00f, 466.16f, 493.88f,
    /* octave 4 */
    523.25f, 554.37f, 587.33f, 622.25f, 659.26f, 698.46f, 739.99f, 783.99f, 830.61f, 880.00f, 932.33f, 987.77f,
    /* octave 5 */
    1046.50f, 1108.73f, 1174.66f, 1244.51f, 1318.51f, 1396.91f, 1479.98f, 1567.98f, 1661.22f, 1760.00f, 1864.66f, 1975.53f,
    /* octave 6 */
    2093.00f, 2217.46f, 2349.32f, 2489.02f, 2637.02f, 2793.83f, 2959.96f, 3135.96f, 3322.44f, 3520.00f, 3729.31f, 3951.07f,
    /* octave 7 */
    4186.01f, 4434.92f, 4698.64f, 4978.03f, 5274.04f, 5587.65f, 5919.91f, 6271.93f, 6644.88f, 7040.00f, 7458.62f, 7902.13f,
    /* fréquence maxi */
    8346.00f
};

// donne le n° de note dans freqs en fonction de l'octave et de la hauteur

inline int fn(int octave, int note) {
    return octave * 12 + note;
}

// transforme une valeur d'une échelle à une autre

inline float mapval(float value, float istart, float istop, float ostart, float ostop) {
    return ostart + (ostop - ostart) * ((value - istart) / (istop - istart));
}

int main(int argc, char** argv) {

    // variables locales
    string fileName; // nom du fichier .txt
    string midFile; // nom du fichier .mid
    int numberOfTracks = 0; // nombre de pistes
    ifstream i; // fichier .txt
    ofstream f; // fichier .mid
    ostringstream head; // entête fichier .mid
    multimap<int, lineData> e; // réservoir d'événements

    // gestion des "hacks"
    bool ja_spectral = false; // mode "ja_spectral"

    // ----------------------------------------
    //  ouverture du fichier txt et chargement
    // ----------------------------------------

    // récupération du fichier source
    if (argc == 2) {
        fileName = argv[1];
        midFile = fileName + ".mid";
    } else {
        // pas de fichier passé en argument
        cout << "usage: txt2mid.exe txtfile.txt" << endl;
        return 0;
    }

    i.open(fileName.c_str(), ifstream::in);
    if (!i) {
        cout << "error: can't open txt file" << endl;
        return 0;
    }

    // fichier .txt ouvert

    while (!i.eof()) {
        lineData r; // données récupérées

        // récupération du type d'événement qui vient
        i >> r.type;
        bool knownType = false; // si type inconnu, reste à false
        r.track = 0; // pas de track entré

        // cas d'une note (N)
        // --------------
        if (r.type == 'N' || r.type == 'n') {
            knownType = true;

            i >> r.start >> r.octave >> r.hauteur >> r.velocite >> r.duree >> r.track;

            // hack ja_spectral
            if (ja_spectral) {
                // décalage
                r.track = (r.track * 2) - 1;

                // note indicative avec ajustement à zéro
                lineData s_info = r;
                s_info.track += 1;
                s_info.hauteur = 11; /* si 4 */
                s_info.octave = 4;
                s_info.miditype = 'o';
                e.insert(pair<int, lineData >(s_info.start, s_info));
                s_info.miditype = 'x';
                e.insert(pair<int, lineData >((s_info.start + s_info.duree), s_info));
            }

            // remise à zéro de la pitchwheel
            r.valeur = 8192;
            r.miditype = 'p';
            e.insert(pair<int, lineData >(r.start, r));

            // remplissage du réservoir d'événements avec NoteOn
            r.miditype = 'o';
            e.insert(pair<int, lineData >(r.start, r));

            // calcul de NoteOff et remplissage du réservoir d'événements avec noteOff
            r.miditype = 'x';
            e.insert(pair<int, lineData >((r.start + r.duree), r));
        }

        // cas d'un contrôleur (C)
        // -------------------
        if (r.type == 'C' || r.type == 'c') {
            knownType = true;

            i >> r.start >> r.controleur >> r.valeur >> r.track;

            // hack ja_spectral
            if (ja_spectral) {
                r.track = (r.track * 2) - 1;
            }

            r.miditype = 'c';
            e.insert(pair<int, lineData >(r.start, r));
        }

        // cas du pitch bend wheel (P)
        // -----------------------
        if (r.type == 'P' || r.type == 'p') {
            knownType = true;

            i >> r.start >> r.valeur >> r.track;

            // hack ja_spectral
            if (ja_spectral) {
                r.track = (r.track * 2) - 1;
            }

            r.miditype = 'p';
            e.insert(pair<int, lineData >(r.start, r));
        }

        // cas d'une fréquence (H)
        // -------------------
        if (r.type == 'H' || r.type == 'h') {
            knownType = true;

            i >> r.start >> r.valeur >> r.velocite >> r.duree >> r.track;

            // bornes maximales et minimales
            if (r.valeur > 8346)
                r.valeur = 8346;
            if (r.valeur < 30)
                r.valeur = 30;

            // trouve la note la plus proche
            float minimum = FLT_MAX;
            int n_min = -1;
            int o_min = -1;

            for (int o = 0; o < 8; o++) {
                for (int n = 0; n < 12; n++) {
                    if (abs(r.valeur - freqs[fn(o, n)]) < minimum) {
                        minimum = abs(r.valeur - freqs[fn(o, n)]);
                        n_min = n;
                        o_min = o;
                    }
                }
            }

            r.octave = o_min;
            r.hauteur = n_min;

            // maper la différence pour trouver la valeur de pitchwheel
            float diff = r.valeur - freqs[fn(o_min, n_min)];
            float pitchwheelval = -1;

            if(diff > 0) {
                // maper contre en haut
                pitchwheelval = mapval(r.valeur, freqs[fn(o_min, n_min)], freqs[fn(o_min, n_min) + 1], 8192, 12287);
            } else {
                // maper contre en bas
                if (fn(o_min, n_min) == 0) {
                    // cas de la note la plus grave
                    pitchwheelval = mapval(r.valeur, 30, freqs[fn(o_min, n_min)], 4096, 8192);
                } else {
                    // autres cas
                    pitchwheelval = mapval(r.valeur, freqs[fn(o_min, n_min) -1], freqs[fn(o_min, n_min)], 4096, 8192);
                }
            }

            r.valeur = pitchwheelval;

            // hack ja_spectral
            if (ja_spectral) {
                // crée de l'espace pour une piste dessous
                // tr 1 => tr 1, tr 2 => tr 3, tr 3 => tr 5, etc.
                r.track = (r.track * 2) - 1;

                // détermine la note indicative à copier sur la piste du dessous
                lineData s_info = r;
                s_info.track += 1;

                s_info.hauteur = 0; // debug
                s_info.octave = 1;

                if (s_info.valeur >= 10239) {
                    s_info.hauteur = 2; /* ré 6 */
                    s_info.octave = 6;
                } else if (s_info.valeur >= 9984 && s_info.valeur <= 10238) {
                    s_info.hauteur = 0; /* do 6 */
                    s_info.octave = 6;
                } else if (s_info.valeur >= 9728 && s_info.valeur <= 9983) {
                    s_info.hauteur = 11; /* si 5 */
                    s_info.octave = 5;
                } else if (s_info.valeur >= 9472 && s_info.valeur <= 9727) {
                    s_info.hauteur = 9; /* la 5 */
                    s_info.octave = 5;
                } else if (s_info.valeur >= 9216 && s_info.valeur <= 9471) {
                    s_info.hauteur = 7; /* sol 5 */
                    s_info.octave = 5;
                } else if (s_info.valeur >= 8960 && s_info.valeur <= 9215) {
                    s_info.hauteur = 5; /* fa 5 */
                    s_info.octave = 5;
                } else if (s_info.valeur >= 8704 && s_info.valeur <= 8959) {
                    s_info.hauteur = 4; /* mi 5 */
                    s_info.octave = 5;
                } else if (s_info.valeur >= 8448 && s_info.valeur <= 8703) {
                    s_info.hauteur = 2; /* ré 5 */
                    s_info.octave = 5;
                } else if (s_info.valeur >= 8193 && s_info.valeur <= 8447) {
                    s_info.hauteur = 0; /* do 5 */
                    s_info.octave = 5;
                } else if (s_info.valeur == 8192) {
                    s_info.hauteur = 11; /* si 4 */
                    s_info.octave = 4;
                } else if (s_info.valeur >= 7937 && s_info.valeur <= 8191) {
                    s_info.hauteur = 9; /* la 4 */
                    s_info.octave = 4;
                } else if (s_info.valeur >= 7681 && s_info.valeur <= 7936) {
                    s_info.hauteur = 7; /* sol 4 */
                    s_info.octave = 4;
                } else if (s_info.valeur >= 7425 && s_info.valeur <= 7680) {
                    s_info.hauteur = 5; /* fa 4 */
                    s_info.octave = 4;
                } else if (s_info.valeur >= 7169 && s_info.valeur <= 7424) {
                    s_info.hauteur = 4; /* mi 4 */
                    s_info.octave = 4;
                } else if (s_info.valeur >= 6913 && s_info.valeur <= 7168) {
                    s_info.hauteur = 2; /* ré 4 */
                    s_info.octave = 4;
                } else if (s_info.valeur >= 6657 && s_info.valeur <= 6912) {
                    s_info.hauteur = 0; /* do 4 */
                    s_info.octave = 4;
                } else if (s_info.valeur >= 6401 && s_info.valeur <= 6656) {
                    s_info.hauteur = 11; /* si 3 */
                    s_info.octave = 3;
                } else if (s_info.valeur >= 6145 && s_info.valeur <= 6400) {
                    s_info.hauteur = 9; /* la 3 */
                    s_info.octave = 3;
                } else if (s_info.valeur <= 6144) {
                    s_info.hauteur = 7; /* sol 3 */
                    s_info.octave = 3;
                }

                // inscrire la note
                s_info.miditype = 'o';
                e.insert(pair<int, lineData >(s_info.start, s_info));
                s_info.miditype = 'x';
                e.insert(pair<int, lineData >((s_info.start + s_info.duree), s_info));
            }

            // remplissage du réservoir d'événements avec NoteOn
            r.miditype = 'o';
            e.insert(pair<int, lineData >(r.start, r));

            // calcul de NoteOff et remplissage du réservoir d'événements avec noteOff
            r.miditype = 'x';
            e.insert(pair<int, lineData >((r.start + r.duree), r));

            // ajout du réhaussement pitchbend wheel
            r.miditype = 'p';
            e.insert(pair<int, lineData >(r.start, r));
        }

        // cas d'un commentaire (_ commentaire _)
        // --------------------
        if (r.type == '_') {
            knownType = true;
            bool fin_commentaire = false;
            // ignore les mots entrés jusqu'au underscore suivant
            while (!fin_commentaire) {
                string rem;
                i >> rem;
                if (rem.compare("_") == 0) {
                    fin_commentaire = true;
                }
            }

        }

        // cas d'un "hack" (# ...)
        // ---------------
        if (r.type == '#') {
            knownType = true;
            string hack;
            i >> hack;

            if (hack.compare("ja_spectral") == 0) {
                ja_spectral = true;
            }
        }

        // cas inconnu
        // -----------
        if (!knownType) {
            cout << "error: unhandled event type (or encoding ?)" << endl;
            return 0;
        }

        // mise à jour du nombre de tracks si nécessaire
        if (r.track > numberOfTracks) {
            numberOfTracks = r.track;

            // hack ja_spectral
            if (ja_spectral) {
                // le nombre de tracks est toujours multiple de 2 dans ce cas là.
                numberOfTracks += (numberOfTracks % 2);
            }
        }
    }

    // fermeture du fichier .txt
    i.close();

    cout << "txt input : ok" << endl;

    // ---------------------------
    //  génération du fichier mid
    // ---------------------------

    f.open(midFile.c_str(), ofstream::binary);
    if (!f) {
        cout << "error: can't write mid file" << endl;
        return 0;
    }

    // génération de l'entête
    head << (char) 'M' << (char) 'T' << (char) 'h' << (char) 'd'
            << (char) 0x00 << (char) 0x00 << (char) 0x00 << (char) 0x06
            << (char) 0x00 << (char) 0x01;

    // nombre de tracks (calcul selon deux bytes puis écriture)
    char tn[2];
    tn[0] = static_cast<char> (numberOfTracks / 256);
    tn[1] = static_cast<char> (numberOfTracks % 256);
    head.write((char*) &tn, 2);

    // vitesse (240 ticks pour une noire)
    head.width(1);
    head << (char) 0x00 << (char) 0xF0;

    // écriture dans le fichier
    f.write(head.str().c_str(), head.str().size());

    // pour chaque piste enregistrée
    for (int t = 0; t < numberOfTracks; t++) {

        ostringstream oss; // contenu
        ostringstream oth; // entête

        int lastTime = 0;
        int deltaTime = 0;

        multimap<int, lineData>::iterator it;

        for (it = e.begin(); it != e.end(); it++) {
            if ((*it).second.track == (t + 1)) {
                // calcul et inscription du temps relatif en ticks
                deltaTime = (*it).first - lastTime;
                lastTime = (*it).first;

                register unsigned long buffer;
                buffer = deltaTime & 0x7F;

                while ((deltaTime >>= 7)) {
                    buffer <<= 8;
                    buffer |= ((deltaTime & 0x7F) | 0x80);
                }

                int n_octets = 0;

                while (true) {
                    oss << static_cast<char> (buffer);
                    if (buffer & 0x80) {
                        buffer >>= 8;
                        n_octets++;
                    } else
                        break;
                }

                if (n_octets > 4) {
                    cout << "error : midi track too long" << endl;
                    return 0;
                }

                // NoteON
                // ------
                if ((*it).second.miditype == 'o') {
                    oss << (char) 0x90;
                    // n° de note
                    oss << static_cast<char> ((*it).second.hauteur + (((*it).second.octave + 1) * 12));
                    // vélocité
                    oss << static_cast<char> ((*it).second.velocite);
                }

                // NoteOFF
                // -------
                if ((*it).second.miditype == 'x') {
                    oss << (char) 0x80;
                    // n° de note
                    oss << static_cast<char> ((*it).second.hauteur + (((*it).second.octave + 1) * 12));
                    // vélocité
                    oss << static_cast<char> ((*it).second.velocite);
                }

                // contrôleur
                // ----------
                if ((*it).second.miditype == 'c') {
                    oss << (char) 0xB0;
                    // n° de contrôleur
                    oss << static_cast<char> ((*it).second.controleur);
                    // valeur de contrôleur
                    oss << static_cast<char> ((*it).second.valeur);
                }

                // pitch bend wheel
                // ----------------
                if ((*it).second.miditype == 'p') {
                    oss << (char) 0xE0;

                    // convertir la valeur en deux octets remplis à 7 bits chacun
                    char right = static_cast<char> (floor((*it).second.valeur / 128));
                    char left = static_cast<char> ((*it).second.valeur % 128);

                    oss << left << right;
                }

            }
        }

        // fin du track
        oss << (char) 0x00 << (char) 0xFF << (char) 0x2F << (char) 0x00;

        // entête du track
        oth << "MTrk";

        // calcul de la taille selon 4 bytes
        char tmp[4];
        tmp[0] = static_cast<char> (floor(oss.str().size() / pow(256, 3)));
        tmp[1] = static_cast<char> (floor(oss.str().size() / pow(256, 2)));
        tmp[2] = static_cast<char> (floor(oss.str().size() / 256));
        tmp[3] = static_cast<char> (oss.str().size() % 256);

        oth.write((char*) &tmp, 4);

        // écriture dans le fichier
        f.write(oth.str().c_str(), oth.str().size());
        f.write(oss.str().c_str(), oss.str().size());
    }

    // fermeture du fichier .mid
    f.close();

    cout << "mid output : ok" << endl;

    return 0;
}

// eof
