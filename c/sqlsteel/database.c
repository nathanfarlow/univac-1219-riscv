/*
 * Database Operations Module
 * Handles initialization, data loading, and basic operations
 */

#include "sqlsteel.h"

/* Global in-memory database */
ElementRecord g_database[MAX_RECORDS];
sql_int_t g_record_count = 0;

/* Initialize database */
void init_database(void) {
    int i;
    for (i = 0; i < MAX_RECORDS; i++) {
        g_database[i].active = 0;
        g_database[i].element_number = 0;
    }
    g_record_count = 0;
}

/* Periodic table data structure */
typedef struct {
    int number;
    const char *name;
    const char *symbol;
    double weight;
    const char *series;
} PeriodicElement;

/* First 118 elements of the periodic table */
static const PeriodicElement periodic_table[] = {
    {1, "HYDROGEN", "H", 1.008, "DIATOMIC NONMETAL"},
    {2, "HELIUM", "He", 4.003, "NOBLE GAS"},
    {3, "LITHIUM", "Li", 6.941, "ALKALI METAL"},
    {4, "BERYLLIUM", "Be", 9.012, "ALKALINE EARTH METAL"},
    {5, "BORON", "B", 10.81, "METALLOID"},
    {6, "CARBON", "C", 12.01, "POLYATOMIC NONMETAL"},
    {7, "NITROGEN", "N", 14.01, "DIATOMIC NONMETAL"},
    {8, "OXYGEN", "O", 16.00, "DIATOMIC NONMETAL"},
    {9, "FLUORINE", "F", 19.00, "DIATOMIC NONMETAL"},
    {10, "NEON", "Ne", 20.18, "NOBLE GAS"},
    {11, "SODIUM", "Na", 22.99, "ALKALI METAL"},
    {12, "MAGNESIUM", "Mg", 24.31, "ALKALINE EARTH METAL"},
    {13, "ALUMINUM", "Al", 26.98, "POST-TRANSITION METAL"},
    {14, "SILICON", "Si", 28.09, "METALLOID"},
    {15, "PHOSPHORUS", "P", 30.97, "POLYATOMIC NONMETAL"},
    {16, "SULFUR", "S", 32.07, "POLYATOMIC NONMETAL"},
    {17, "CHLORINE", "Cl", 35.45, "DIATOMIC NONMETAL"},
    {18, "ARGON", "Ar", 39.95, "NOBLE GAS"},
    {19, "POTASSIUM", "K", 39.10, "ALKALI METAL"},
    {20, "CALCIUM", "Ca", 40.08, "ALKALINE EARTH METAL"},
    {21, "SCANDIUM", "Sc", 44.96, "TRANSITION METAL"},
    {22, "TITANIUM", "Ti", 47.88, "TRANSITION METAL"},
    {23, "VANADIUM", "V", 50.94, "TRANSITION METAL"},
    {24, "CHROMIUM", "Cr", 52.00, "TRANSITION METAL"},
    {25, "MANGANESE", "Mn", 54.94, "TRANSITION METAL"},
    {26, "IRON", "Fe", 55.85, "TRANSITION METAL"},
    {27, "COBALT", "Co", 58.93, "TRANSITION METAL"},
    {28, "NICKEL", "Ni", 58.69, "TRANSITION METAL"},
    {29, "COPPER", "Cu", 63.55, "TRANSITION METAL"},
    {30, "ZINC", "Zn", 65.39, "TRANSITION METAL"},
    {31, "GALLIUM", "Ga", 69.72, "POST-TRANSITION METAL"},
    {32, "GERMANIUM", "Ge", 72.61, "METALLOID"},
    {33, "ARSENIC", "As", 74.92, "METALLOID"},
    {34, "SELENIUM", "Se", 78.96, "POLYATOMIC NONMETAL"},
    {35, "BROMINE", "Br", 79.90, "DIATOMIC NONMETAL"},
    {36, "KRYPTON", "Kr", 83.80, "NOBLE GAS"},
    {37, "RUBIDIUM", "Rb", 85.47, "ALKALI METAL"},
    {38, "STRONTIUM", "Sr", 87.62, "ALKALINE EARTH METAL"},
    {39, "YTTRIUM", "Y", 88.91, "TRANSITION METAL"},
    {40, "ZIRCONIUM", "Zr", 91.22, "TRANSITION METAL"},
    {41, "NIOBIUM", "Nb", 92.91, "TRANSITION METAL"},
    {42, "MOLYBDENUM", "Mo", 95.94, "TRANSITION METAL"},
    {43, "TECHNETIUM", "Tc", 98.00, "TRANSITION METAL"},
    {44, "RUTHENIUM", "Ru", 101.1, "TRANSITION METAL"},
    {45, "RHODIUM", "Rh", 102.9, "TRANSITION METAL"},
    {46, "PALLADIUM", "Pd", 106.4, "TRANSITION METAL"},
    {47, "SILVER", "Ag", 107.9, "TRANSITION METAL"},
    {48, "CADMIUM", "Cd", 112.4, "TRANSITION METAL"},
    {49, "INDIUM", "In", 114.8, "POST-TRANSITION METAL"},
    {50, "TIN", "Sn", 118.7, "POST-TRANSITION METAL"},
    {51, "ANTIMONY", "Sb", 121.8, "METALLOID"},
    {52, "TELLURIUM", "Te", 127.6, "METALLOID"},
    {53, "IODINE", "I", 126.9, "DIATOMIC NONMETAL"},
    {54, "XENON", "Xe", 131.3, "NOBLE GAS"},
    {55, "CESIUM", "Cs", 132.9, "ALKALI METAL"},
    {56, "BARIUM", "Ba", 137.3, "ALKALINE EARTH METAL"},
    {57, "LANTHANUM", "La", 138.9, "LANTHANIDE"},
    {58, "CERIUM", "Ce", 140.1, "LANTHANIDE"},
    {59, "PRASEODYMIUM", "Pr", 140.9, "LANTHANIDE"},
    {60, "NEODYMIUM", "Nd", 144.2, "LANTHANIDE"},
    {61, "PROMETHIUM", "Pm", 145.0, "LANTHANIDE"},
    {62, "SAMARIUM", "Sm", 150.4, "LANTHANIDE"},
    {63, "EUROPIUM", "Eu", 152.0, "LANTHANIDE"},
    {64, "GADOLINIUM", "Gd", 157.25, "LANTHANIDE"},
    {65, "TERBIUM", "Tb", 158.9, "LANTHANIDE"},
    {66, "DYSPROSIUM", "Dy", 162.5, "LANTHANIDE"},
    {67, "HOLMIUM", "Ho", 164.9, "LANTHANIDE"},
    {68, "ERBIUM", "Er", 167.3, "LANTHANIDE"},
    {69, "THULIUM", "Tm", 168.9, "LANTHANIDE"},
    {70, "YTTERBIUM", "Yb", 173.0, "LANTHANIDE"},
    {71, "LUTETIUM", "Lu", 175.0, "LANTHANIDE"},
    {72, "HAFNIUM", "Hf", 178.5, "TRANSITION METAL"},
    {73, "TANTALUM", "Ta", 180.9, "TRANSITION METAL"},
    {74, "TUNGSTEN", "W", 183.8, "TRANSITION METAL"},
    {75, "RHENIUM", "Re", 186.2, "TRANSITION METAL"},
    {76, "OSMIUM", "Os", 190.2, "TRANSITION METAL"},
    {77, "IRIDIUM", "Ir", 192.2, "TRANSITION METAL"},
    {78, "PLATINUM", "Pt", 195.1, "TRANSITION METAL"},
    {79, "GOLD", "Au", 197.0, "TRANSITION METAL"},
    {80, "MERCURY", "Hg", 200.6, "TRANSITION METAL"},
    {81, "THALLIUM", "Tl", 204.4, "POST-TRANSITION METAL"},
    {82, "LEAD", "Pb", 207.2, "POST-TRANSITION METAL"},
    {83, "BISMUTH", "Bi", 209.0, "POST-TRANSITION METAL"},
    {84, "POLONIUM", "Po", 209.0, "POST-TRANSITION METAL"},
    {85, "ASTATINE", "At", 210.0, "METALLOID"},
    {86, "RADON", "Rn", 222.0, "NOBLE GAS"},
    {87, "FRANCIUM", "Fr", 223.0, "ALKALINE METAL"},
    {88, "RADIUM", "Ra", 226.0, "ALKALINE EARTH METAL"},
    {89, "ACTINIUM", "Ac", 227.0, "ACTINIDE"},
    {90, "THORIUM", "Th", 232.0, "ACTINIDE"},
    {91, "PROTACTINIUM", "Pa", 231.0, "ACTINIDE"},
    {92, "URANIUM", "U", 238.0, "ACTINIDE"},
    {93, "NEPTUNIUM", "Np", 237.0, "ACTINIDE"},
    {94, "PLUTONIUM", "Pu", 244.0, "ACTINIDE"},
    {95, "AMERICIUM", "Am", 243.0, "ACTINIDE"},
    {96, "CURIUM", "Cm", 247.0, "ACTINIDE"},
    {97, "BERKELIUM", "Bk", 247.0, "ACTINIDE"},
    {98, "CALIFORNIUM", "Cf", 251.0, "ACTINIDE"},
    {99, "EINSTEINIUM", "Es", 252.0, "ACTINIDE"},
    {100, "FERMIUM", "Fm", 257.0, "ACTINIDE"},
    {101, "MENDELEVIUM", "Md", 258.0, "ACTINIDE"},
    {102, "NOBELIUM", "No", 259.0, "ACTINIDE"},
    {103, "LAWRENCIUM", "Lr", 262.0, "ACTINIDE"},
    {104, "RUTHERFORDIUM", "Rf", 267.0, "TRANSITION METAL"},
    {105, "DUBNIUM", "Db", 270.0, "TRANSITION METAL"},
    {106, "SEABORGIUM", "Sg", 269.0, "TRANSITION METAL"},
    {107, "BOHRIUM", "Bh", 270.0, "TRANSITION METAL"},
    {108, "HASSIUM", "Hs", 277.0, "TRANSITION METAL"},
    {109, "MEITNERIUM", "Mt", 278.0, "TRANSITION METAL"},
    {110, "DARMSTADTIUM", "Ds", 281.0, "TRANSITION METAL"},
    {111, "ROENTGENIUM", "Rg", 282.0, "TRANSITION METAL"},
    {112, "COPERNICIUM", "Cn", 285.0, "TRANSITION METAL"},
    {113, "NIHONIUM", "Nh", 286.0, "TRANSITION METAL"},
    {114, "FLEROVIUM", "Fl", 289.0, "POST-TRANSITION METAL"},
    {115, "MOSCOVIUM", "Mc", 290.0, "POST-TRANSITION METAL"},
    {116, "LIVERMORIUM", "Lv", 293.0, "POST-TRANSITION METAL"},
    {117, "TENNESSINE", "Ts", 294.0, "METALLOID"},
    {118, "OGANESSON", "Og", 294.0, "NOBLE GAS"}
};

/* Load periodic table data */
void load_demo_data(void) {
    int i;
    int table_size = sizeof(periodic_table) / sizeof(periodic_table[0]);
    
    for (i = 0; i < table_size && i < MAX_RECORDS; i++) {
        g_database[i].element_number = periodic_table[i].number;
        g_database[i].active = 1;

        /* Copy element data */
        PLATFORM_STRCPY(g_database[i].element_name, MAX_NAME_LEN, periodic_table[i].name);
        PLATFORM_STRCPY(g_database[i].symbol, MAX_SYMBOL_LEN, periodic_table[i].symbol);
        g_database[i].atomic_weight = periodic_table[i].weight;
        PLATFORM_STRCPY(g_database[i].series, MAX_SERIES_LEN, periodic_table[i].series);
    }
    
    g_record_count = table_size;
    printf("DATABASE INITIALIZED: %d ELEMENTS LOADED FROM PERIODIC TABLE\n", table_size);
}

/* No longer needed - kept for compatibility */
void generate_random_name(char *dest, int max_len) {
    PLATFORM_STRCPY(dest, max_len, "N/A");
}

/* Get count of active records */
int get_active_count(void) {
    int i, count = 0;
    for (i = 0; i < MAX_RECORDS; i++) {
        if (g_database[i].active) count++;
    }
    return count;
}
