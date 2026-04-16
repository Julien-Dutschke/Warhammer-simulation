#include <iostream>
#include <vector>
#include <random>

using namespace std;

struct Cible
{
    int endurance;
    int save;                  
    int invulnerableSave_value; 
    int fnp_value;             
};

struct ParametresArme {
    bool rerollTouche;
    bool rerollWound;
    bool lethal_hit;
    int critical_hit;
    bool sustain_hit;
    bool devastating_wound;
    int sustain_hit_v;
    int critical_wound;
};

struct Arme {
    int nbAttaques;
    int ct; 
    int force;
    int ap;
    int degats;
    ParametresArme param;
};

struct ResultatsTouche {
    int normales;
    int lethal;
    int sustained;
    int loupees;
    int totalTouchesInitiales;
    int totalFinal;
    double pourcentage;
};

struct ResultatsBlessure {
    int blessuresNormales;
    int blessuresMortelles;
    int totalBlessures;
};

struct ResultatsFinaux
{
    int degatsSubis;
    int sauvegardesReussies;
    int degatsBloquesParFNP;
};

mt19937 gen(random_device{}());

ResultatsTouche simulerTouche(const Arme& arme) {
    int touchesNormales = 0;
    int sustainedHits = 0;
    int lethalHits = 0;

    uniform_int_distribution<int> dist(1, 6);

    cout << "--- Phase de Touche ---" << endl;

    for(int i = 0; i < arme.nbAttaques; i++) {
        int dice = dist(gen);
        bool succes = (dice >= arme.ct);

        if (!succes && arme.param.rerollTouche) {
            dice = dist(gen);
            succes = (dice >= arme.ct);
        }

        if (succes) {
            if (arme.param.lethal_hit && dice >= arme.param.critical_hit)
                lethalHits++;
            else 
                touchesNormales++;

            if (arme.param.sustain_hit && dice >= arme.param.critical_hit)
                sustainedHits += arme.param.sustain_hit_v; 
        }
    }
    
    int totalTouchesInitiales = touchesNormales + lethalHits;
    int totalFinal = totalTouchesInitiales + sustainedHits;
    double pourcentage = ((double)totalTouchesInitiales / arme.nbAttaques) * 100.0;

    cout << " - Normales : " << touchesNormales << endl;
    cout << " - Lethal Hits : " << lethalHits << endl;
    cout << " - Sustained Hits : " << sustainedHits << endl;
    cout << " - Touches loupees : " << (arme.nbAttaques - totalTouchesInitiales) << endl;
    cout << "Total touches generees : " << totalFinal << endl;
    cout << "Taux de reussite : " << pourcentage << "%\n" << endl;

    return {touchesNormales, lethalHits, sustainedHits, (arme.nbAttaques - totalTouchesInitiales), totalTouchesInitiales, totalFinal, pourcentage};
}

ResultatsBlessure simulerBlessure(ResultatsTouche res, Arme arme, int enduCible, int modificateur = 0) {
    int blessuresNormales = 0;
    int blessuresMortelles = 0;
    
    int scoreRequis = 4;
    if (arme.force >= enduCible * 2) scoreRequis = 2;
    else if (arme.force > enduCible) scoreRequis = 3;
    else if (arme.force <= enduCible / 2) scoreRequis = 6;
    else if (arme.force < enduCible) scoreRequis = 5;
    scoreRequis += modificateur;
    
    uniform_int_distribution<int> dist(1, 6);
    
    // On doit blesser avec TOUTES les touches générées (Initiales + Sustained)
    int totalTouches = res.totalFinal;
    
    for(int i = 0; i < totalTouches; i++) {
        int dice = dist(gen);
        bool succes = (dice >= scoreRequis);
        
        if (!succes && arme.param.rerollWound) {
            dice = dist(gen);
            succes = (dice >= scoreRequis);
        }
        
        if (succes) {
            if (arme.param.devastating_wound && dice >= arme.param.critical_wound)
                blessuresMortelles++;
            else 
                blessuresNormales++;
        }
    }
    
    // Calculs statistiques pour la blessure
    int totalBlessures = blessuresNormales + blessuresMortelles;
    // Taux de blessure par rapport au nombre de touches entrantes
    double pourcentage = (totalTouches > 0) ? ((double)totalBlessures / totalTouches) * 100.0 : 0.0;

    // Affichage des résultats
    cout << "--- Phase de Blessure ---" << endl;
    cout << " - Blessures Normales : " << blessuresNormales << endl;
    cout << " - Blessures Mortelles : " << blessuresMortelles << endl;
    cout << " - Total blessures : " << totalBlessures << endl;
    cout << " - Taux de reussite (blessure) : " << pourcentage << "%" << endl;
    return {blessuresNormales, blessuresMortelles, totalBlessures};
}

// --- Calcul pur (Logique métier) ---
ResultatsFinaux calculerDegats(const ResultatsBlessure& resB, const Cible& cible, const Arme& arme)
{
    int degatsFinal = 0;
    int sauvegardesReussies = 0;
    int degatsBloquesParFNP = 0;
    
    uniform_int_distribution<int> dist(1, 6);

    auto testerFnp = [&](int nbPointsDegats)
    {
        int echecsFnp = 0; 
        for(int i = 0; i < nbPointsDegats; i++)
        {
            if (dist(gen) >= cible.fnp_value)
            {
                echecsFnp++; // Le FNP échoue, le dégât passe
            } else {
                degatsBloquesParFNP++; // Le FNP réussit, le dégât est ignoré
            }
        }
        return echecsFnp;
    };

    
    for(int i = 0; i < resB.blessuresNormales; i++) {
        int saveModifiee = cible.save + arme.ap;
        int seuilSave = (cible.invulnerableSave_value > 0) ? min(saveModifiee, cible.invulnerableSave_value) : saveModifiee;

        // Test de la Sauvegarde (Armure ou Invu)
        if (dist(gen) >= seuilSave)
        { 
            sauvegardesReussies++; // Sauvegarde réussie
        } else {
            // Sauvegarde échouée : on teste le FNP pour chaque point de dégât
            if (cible.fnp_value > 0) {
                degatsFinal += testerFnp(arme.degats);
            } else {
                degatsFinal += arme.degats;
            }
        }
    }

    for(int i = 0; i < resB.blessuresMortelles; i++) {
        if (cible.fnp_value > 0) {
            degatsFinal += testerFnp(1); // 1 dégât par mortelle
        } else {
            degatsFinal += 1;
        }
    }
    cout << "\n\n--- Phase de Sauvegarde & FNP ---" << endl;
    cout << " - Dégâts maximum théoriques (sans sauvegarde ni FNP) : "
         << resB.totalBlessures *arme.degats  << endl;
    cout << " - Blessures bloquées par la Sauvegarde : " << sauvegardesReussies << endl;
    cout << " - Dégâts annulés par le FNP            : " << degatsBloquesParFNP << endl;
    cout << " - Dégâts réellement infligés          : " << degatsFinal << endl;
    return {degatsFinal, sauvegardesReussies, degatsBloquesParFNP};
}





ResultatsFinaux simulerSauvegarde(const ResultatsBlessure& resB, const Cible& cible, const Arme& arme)
{
    ResultatsFinaux res = calculerDegats(resB, cible, arme);
    return res;
}

int main()
{
    Cible spaceMarine =
    { 
    4, // endurance
    3, // save ()
    0, // invulnerableSave_value (0)
    0  // fnp_value (5+)
    };
    Arme monArme =
    {
        10, // nbAttaques : Nombre de dés lancés pour toucher
        3,  // ct : Compétence de Tir (le score requis sur 1d6)
        6,  // force : Force de l'arme
        2,  // ap : Pénétration d'armure (Armor Penetration)
        1, //  Dégat
        
        // ParametresArme :
        {
            false, // rerollTouche : Relance les échecs au jet de touche
            false, // rerollWound : Relance les échecs au jet de blessure
            false, // lethal_hit : Les critiques blessent automatiquement (Lethal Hits)
            6,     // critical_hit : Valeur du dé pour considérer un coup comme Critique
            false, // sustain_hit : Génère des touches bonus en cas de Critique
            false, // devastating_wound : Les critiques deviennent des Blessures Mortelles
            1,     // sustain_hit_v : Nombre de touches bonus générées par Sustain Hit
            6      // critical_wound : Valeur du dé pour considérer un coup comme critique en phase de Blessure
        }
    };

    // Lancement de la simulation
    ResultatsTouche resT = simulerTouche(monArme);
    
    // On passe l'arme, le résultat de la touche, et l'endurance (ici 4)
    ResultatsBlessure resB = simulerBlessure(resT, monArme, spaceMarine.endurance);
    
    simulerSauvegarde(resB, spaceMarine, monArme);

    return 0;
}
