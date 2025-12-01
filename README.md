# 🧪 מערכות הפעלה - תרגיל 2: ספירת מולקולות
## Operating Systems Assignment 2 - Molecular Warehouse System

---

## 📋 מחברים / Authors
- **עידו כהן** - Ido Cohen
- **איתן חלימי** - Eitan Halimi

---

## 🎯 סקירת הפרויקט / Project Overview

פרויקט זה מממש **מערכת ניהול מחסן מולקולות מתקדמת** המדגימה מושגים מתקדמים בתכנות מערכות, תקשורת רשתית וקואורדינציה בין תהליכים. המערכת מתפתחת דרך **6 שלבים פרוגרסיביים**, כאשר כל שלב מוסיף יכולות חדשות על גבי השלב הקודם.

This project implements a sophisticated **molecular warehouse management system** that demonstrates advanced concepts in systems programming, network communication, and inter-process coordination. The system evolves through **6 progressive stages**, where each stage builds upon and extends the previous one.

### 🔬 המחסן מנהל:
- **שלושה סוגי אטומים**: פחמן (Carbon), מימן (Hydrogen), חמצן (Oxygen)
- **ארבע מולקולות**: H₂O (מים), CO₂ (פחמן דו-חמצני), C₂H₆O (אלכוהול), C₆H₁₂O₆ (גלוקוז)
- **שלושה משקאות**: Soft Drink, Vodka, Champagne

### 🔧 יכולות מרכזיות:
| יכולת | תיאור |
|-------|--------|
| **תקשורת מרובת פרוטוקולים** | TCP, UDP, Unix Domain Sockets |
| **עיבוד מקבילי** | טיפול במספר לקוחות בו-זמנית |
| **אחסון מתמיד** | קבצים ממופים לזיכרון עם סנכרון תהליכים |
| **מלאי בזמן אמת** | ספירת אטומים עם הגנה מפני גלישה (עד 10¹⁸) |
| **ייצור משקאות** | חישוב כמויות משקאות לפי מתכונים כימיים |

---

## 📁 מבנה הפרויקט / Project Structure

```
OS_2/
├── q1/                        # שלב 1 - מחסן אטומים (TCP בסיסי)
│   ├── atom_warehouse.c       # שרת TCP לניהול אטומים
│   ├── atom_supplier.c        # לקוח TCP אינטראקטיבי
│   └── Makefile
├── q2/                        # שלב 2 - בקשת מולקולות (+UDP)
│   ├── molecule_supplier.c    # שרת TCP+UDP
│   ├── molecule_requester.c   # לקוח UDP
│   └── Makefile
├── q3/                        # שלב 3 - קונסול (+stdin)
│   ├── drinks_bar.c           # שרת עם ממשק קונסול
│   ├── atom_supplier.c        # לקוח TCP
│   ├── molecule_requester.c   # לקוח UDP
│   └── Makefile
├── q4/                        # שלב 4 - אופציות התחלה (+getopt, timeout)
│   ├── drinks_bar.c           # שרת עם CLI מתקדם
│   ├── atom_supplier.c        # לקוח עם אופציות
│   ├── molecule_requester.c   # לקוח עם אופציות
│   └── Makefile
├── q5/                        # שלב 5 - UDS (+Unix Domain Sockets)
│   ├── drinks_bar.c           # שרת רב-תחבורה
│   ├── atom_supplier.c        # לקוח רב-תחבורה
│   ├── molecule_requester.c   # לקוח רב-תחבורה
│   └── Makefile
├── q6/                        # שלב 6 - מקבוליות תהליכים (+mmap, flock)
│   ├── drinks_bar.c           # שרת עם אחסון מתמיד
│   ├── atom_supplier.c        # לקוח סופי
│   ├── molecule_requester.c   # לקוח סופי
│   ├── coverage_report_q6.txt # דו"ח כיסוי קוד
│   └── Makefile
├── Makefile                   # מערכת בנייה רקורסיבית
├── מטלה.txt                   # מפרט המטלה
└── README.md                  # תיעוד זה
```

---

## 🚀 שלבי הפיתוח - בנייה הדרגתית / Progressive Development Stages

### 📌 הקשר בין השלבים
כל שלב **מרחיב ומוסיף** על השלב הקודם. הפונקציונליות הקודמת נשמרת תמיד:

```
שלב 1 (TCP בסיסי)
    ↓ + UDP
שלב 2 (TCP + UDP)
    ↓ + stdin קונסול
שלב 3 (TCP + UDP + קונסול)
    ↓ + getopt + timeout
שלב 4 (CLI מתקדם)
    ↓ + UDS
שלב 5 (רב-תחבורה)
    ↓ + mmap + flock
שלב 6 (מקבוליות + אחסון מתמיד)
```

---

## 📗 שלב 1 - מחסן אטומים (15 נקודות)
### Stage 1 - Atom Warehouse (15 points)

**קבצים**: `atom_warehouse.c`, `atom_supplier.c`

### 🆕 מה חדש בשלב זה:
- **שרת TCP בסיסי** - מאזין לחיבורים ומנהל מלאי אטומים
- **I/O Multiplexing** - שימוש ב-`select()` לטיפול בלקוחות מרובים
- **ניהול מלאי** - מעקב אחר Carbon, Hydrogen, Oxygen (עד 10¹⁸)

### פקודות נתמכות:
```
ADD CARBON <כמות>
ADD HYDROGEN <כמות>
ADD OXYGEN <כמות>
```

### הרצה:
```bash
# Terminal 1 - שרת
cd q1
./atom_warehouse 12345

# Terminal 2 - לקוח
./atom_supplier localhost 12345
```

### טכנולוגיות:
- `socket()`, `bind()`, `listen()`, `accept()`
- `select()` - I/O multiplexing
- `recv()`, `send()` - תקשורת TCP

---

## 📘 שלב 2 - בקשת מולקולות (15 נקודות)
### Stage 2 - Molecule Requests (15 points)

**קבצים**: `molecule_supplier.c`, `molecule_requester.c`

### 🆕 מה חדש בשלב זה (מעבר לשלב 1):
- **תמיכה ב-UDP** - לבקשות מולקולות
- **הפחתת אטומים** - יצירת מולקולות מהמלאי
- **ארבע מולקולות**:

| מולקולה | נוסחה | אטומים נדרשים |
|---------|-------|---------------|
| WATER | H₂O | 2H + 1O |
| CARBON DIOXIDE | CO₂ | 1C + 2O |
| ALCOHOL | C₂H₆O | 2C + 6H + 1O |
| GLUCOSE | C₆H₁₂O₆ | 6C + 12H + 6O |

### פקודות חדשות (UDP):
```
DELIVER WATER <כמות>
DELIVER CARBON DIOXIDE <כמות>
DELIVER ALCOHOL <כמות>
DELIVER GLUCOSE <כמות>
```

### הרצה:
```bash
# Terminal 1 - שרת (TCP פורט 12345, UDP פורט 12346)
cd q2
./molecule_supplier 12345 12346

# Terminal 2 - לקוח UDP
./molecule_requester localhost 12346
```

### טכנולוגיות חדשות:
- `SOCK_DGRAM` - UDP sockets
- `recvfrom()`, `sendto()` - תקשורת UDP
- ניהול מרובה פרוטוקולים עם `select()`

---

## 📙 שלב 3 - קונסול (15 נקודות)
### Stage 3 - Console Interface (15 points)

**קבצים**: `drinks_bar.c`, `atom_supplier.c`, `molecule_requester.c`

### 🆕 מה חדש בשלב זה (מעבר לשלב 2):
- **ממשק קונסול** - קלט מהמקלדת במקביל ללקוחות
- **חישוב משקאות** - כמה משקאות ניתן לייצר מהמלאי
- **שלושה מתכונים**:

| משקה | מרכיבים | אטומים לכל יחידה |
|------|---------|------------------|
| SOFT DRINK | H₂O + CO₂ + C₆H₁₂O₆ | 7C + 14H + 9O |
| VODKA | H₂O + C₂H₆O + C₆H₁₂O₆ | 8C + 20H + 8O |
| CHAMPAGNE | H₂O + CO₂ + C₂H₆O | 3C + 8H + 4O |

### פקודות קונסול חדשות:
```
GEN SOFT DRINK    # כמה משקאות קלים ניתן לייצר
GEN VODKA         # כמה וודקה ניתן לייצר
GEN CHAMPAGNE     # כמה שמפניה ניתן לייצר
exit / quit       # יציאה
```

### הרצה:
```bash
cd q3
./drinks_bar 12345 12346
# עכשיו ניתן להקליד פקודות GEN במסוף השרת
```

### טכנולוגיות חדשות:
- `STDIN_FILENO` ב-`select()` - האזנה למקלדת
- חישוב מינימום - מציאת גורם מגביל

---

## 📕 שלב 4 - אופציות התחלה (20 נקודות)
### Stage 4 - Startup Options (20 points)

**קבצים**: `drinks_bar.c`, `atom_supplier.c`, `molecule_requester.c`

### 🆕 מה חדש בשלב זה (מעבר לשלב 3):
- **פרמטרים בשורת פקודה** - `getopt_long()`
- **אתחול מלאי** - התחלה עם כמות ידועה של אטומים
- **Timeout** - סגירה אוטומטית לאחר חוסר פעילות
- **ארגומנטים בכל סדר** - גמישות מלאה

### אופציות השרת:
| אופציה | תיאור | חובה/רשות |
|--------|--------|-----------|
| `-T, --tcp-port` | פורט TCP | **חובה** |
| `-U, --udp-port` | פורט UDP | **חובה** |
| `-o, --oxygen` | אטומי חמצן התחלתיים | רשות |
| `-c, --carbon` | אטומי פחמן התחלתיים | רשות |
| `-h, --hydrogen` | אטומי מימן התחלתיים | רשות |
| `-t, --timeout` | Timeout בשניות | רשות |

### אופציות הלקוח:
| אופציה | תיאור |
|--------|--------|
| `-h` | כתובת השרת (hostname/IP) |
| `-p` | פורט השרת |

### דוגמאות הרצה:
```bash
# שרת עם מלאי התחלתי ו-timeout
cd q4
./drinks_bar -T 12345 -U 12346 -c 1000 -o 2000 -h 3000 -t 60

# לקוח
./atom_supplier -h localhost -p 12345
./molecule_requester -h 127.0.0.1 -p 12346
```

### טכנולוגיות חדשות:
- `getopt_long()` - פרסור ארגומנטים
- `signal(SIGALRM, handler)` - טיפול בסיגנלים
- `alarm()` - מנגנון timeout

---

## 📓 שלב 5 - UDS (15 נקודות)
### Stage 5 - Unix Domain Sockets (15 points)

**קבצים**: `drinks_bar.c`, `atom_supplier.c`, `molecule_requester.c`

### 🆕 מה חדש בשלב זה (מעבר לשלב 4):
- **Unix Domain Sockets** - תקשורת מקומית מהירה
- **UDS Stream** - תחליף ל-TCP (לאטומים)
- **UDS Datagram** - תחליף ל-UDP (למולקולות)
- **ניקוי קבצי socket** - ניהול מחזור חיים

### אופציות UDS חדשות בשרת:
| אופציה | תיאור |
|--------|--------|
| `-s, --stream-path` | נתיב socket מסוג stream |
| `-d, --datagram-path` | נתיב socket מסוג datagram |

### אופציות UDS חדשות בלקוח:
| אופציה | תיאור |
|--------|--------|
| `-f` | נתיב לקובץ UDS socket |

### דוגמאות הרצה:
```bash
cd q5

# Option 1: רשת (TCP/UDP)
./drinks_bar -T 12345 -U 12346 -c 1000

# Option 2: UDS
./drinks_bar -s /tmp/stream.sock -d /tmp/dgram.sock -c 1000

# לקוח UDS
./atom_supplier -f /tmp/stream.sock
./molecule_requester -f /tmp/dgram.sock
```

### 🚫 שגיאה: ארגומנטים סותרים
```bash
# לא ניתן לציין גם רשת וגם UDS:
./atom_supplier -h localhost -p 12345 -f /tmp/stream.sock  # שגיאה!
```

### טכנולוגיות חדשות:
- `AF_UNIX` - Unix Domain Sockets
- `struct sockaddr_un` - כתובות UDS
- `unlink()` - ניקוי קבצי socket

---

## 📔 שלב 6 - מקבוליות תהליכים (20 נקודות)
### Stage 6 - Process Concurrency (20 points)

**קבצים**: `drinks_bar.c`, `atom_supplier.c`, `molecule_requester.c`

### 🆕 מה חדש בשלב זה (מעבר לשלב 5):
- **שמירת מצב בקובץ** - אחסון מתמיד של המלאי
- **Memory Mapping** - `mmap()` לגישה מהירה
- **נעילת קבצים** - `flock()` לסנכרון בין תהליכים
- **ריצה מקבילית** - מספר שרתים יכולים לעבוד על אותו קובץ

### אופציה חדשה:
| אופציה | תיאור |
|--------|--------|
| `-f, --save-file` | נתיב לקובץ שמירה |

### התנהגות הקובץ:
| מצב | התנהגות |
|-----|---------|
| קובץ קיים | טוען מלאי מהקובץ, **מתעלם** מערכי CLI |
| קובץ חדש | יוצר ומאתחל עם ערכי CLI |
| ללא קובץ | התנהגות רגילה (זיכרון בלבד) |

### אסטרטגיית נעילה:
| פעולה | סוג נעילה |
|-------|-----------|
| הוספת/הפחתת אטומים | `LOCK_EX` (Exclusive) |
| קריאת מלאי/חישוב משקאות | `LOCK_SH` (Shared) |

### דוגמאות הרצה:
```bash
cd q6

# הרצה ראשונה - יצירת קובץ
./drinks_bar -T 12345 -U 12346 -f warehouse.dat -c 5000 -o 3000 -h 7000

# הרצה שנייה (במקביל או לאחר סגירה) - טוענת מהקובץ
./drinks_bar -T 12347 -U 12348 -f warehouse.dat

# הרצה מקבילית על אותו קובץ
# Terminal 1
./drinks_bar -T 12345 -U 12346 -f /tmp/shared.dat -c 1000

# Terminal 2 (מאותו קובץ!)
./drinks_bar -T 12347 -U 12348 -f /tmp/shared.dat
```

### טכנולוגיות חדשות:
- `mmap()` - מיפוי קובץ לזיכרון
- `flock()` - נעילה ייעוצית
- `ftruncate()` - קביעת גודל קובץ
- `MAP_SHARED` - שיתוף בין תהליכים

---

## 🔧 בנייה והרצה / Build & Run

### בניית כל השלבים:
```bash
make all
```

### בניית שלב ספציפי:
```bash
cd q1 && make    # שלב 1
cd q2 && make    # שלב 2
cd q3 && make    # שלב 3
cd q4 && make    # שלב 4
cd q5 && make    # שלב 5
cd q6 && make    # שלב 6
```

### ניקוי:
```bash
make clean
```

---

## 📋 סיכום פקודות / Command Summary

### פקודות TCP (הוספת אטומים):
```
ADD CARBON <amount>
ADD HYDROGEN <amount>
ADD OXYGEN <amount>
```

### פקודות UDP (בקשת מולקולות):
```
DELIVER WATER <amount>
DELIVER CARBON DIOXIDE <amount>
DELIVER ALCOHOL <amount>
DELIVER GLUCOSE <amount>
```

### פקודות קונסול (חישוב משקאות):
```
GEN SOFT DRINK
GEN VODKA
GEN CHAMPAGNE
exit / quit
```

---

## 🧪 כיסוי קוד / Code Coverage

דו"ח כיסוי קוד זמין בקובץ `q6/coverage_report_q6.txt`:

| קובץ | כיסוי |
|------|-------|
| atom_supplier.c | 84.40% |
| drinks_bar.c | 80.43% |
| molecule_requester.c | 84.17% |
| **סה"כ** | **81.79%** |

---

## 🔬 סיכום טכנולוגי / Technical Summary

| שלב | טכנולוגיות מרכזיות |
|-----|-------------------|
| Q1 | TCP sockets, `select()`, I/O multiplexing |
| Q2 | UDP sockets, multi-protocol handling |
| Q3 | stdin handling, drink calculations |
| Q4 | `getopt_long()`, `SIGALRM`, `alarm()` |
| Q5 | `AF_UNIX`, UDS stream/datagram |
| Q6 | `mmap()`, `flock()`, file persistence |

---

## 🎓 מידע על המטלה / Assignment Info

**קורס**: מערכות הפעלה - מדעי המחשב  
**מוסד**: אוניברסיטת אריאל  
**משקל**: 10% מהציון הסופי + 5% הגנה

---

**🔥 פרויקט זה מדגים שליטה במושגים מתקדמים של תכנות מערכות כולל תכנות רשתות, תקשורת בין-תהליכית, ניהול זיכרון ותכנות מקבילי במערכת ניהול מחסן מולקולות ברמה מקצועית.**
