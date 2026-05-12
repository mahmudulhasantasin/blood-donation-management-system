/*
 * ============================================================
 *   BLOOD DONATION MANAGEMENT SYSTEM
 *   Course: Data Structure
 *   Data Structures Used:
 *     - Linked List  : Donor database
 *     - Stack        : Donation history + cooldown check
 *     - Queue        : Patient blood requests
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_HOSPITALS 5
#define COOLDOWN_DAYS 90   /* 3 months */
#define SAVE_FILE     "blood_data.txt"

/* Valid blood groups */
static const char *VALID_BLOOD_GROUPS[] = {
    "A+","A-","B+","B-","O+","O-","AB+","AB-", NULL
};

int is_valid_blood_group(const char *bg) {
    for (int i = 0; VALID_BLOOD_GROUPS[i]; i++)
        if (strcasecmp(bg, VALID_BLOOD_GROUPS[i]) == 0) return 1;
    return 0;
}

/* ============================================================
   STRUCTS
   ============================================================ */

/* Donation history node (used in Stack) */
typedef struct DonationRecord {
    char date[20];           /* "YYYY-MM-DD" */
    char hospital[50];
    struct DonationRecord *next;
} DonationRecord;

/* Stack for donation history */
typedef struct {
    DonationRecord *top;
    int size;
} DonationStack;

/* Donor node (used in Linked List) */
typedef struct Donor {
    int  id;
    char name[50];
    char blood_group[5];     /* A+, B-, O+, AB+, etc. */
    char area[50];
    char hospitals[MAX_HOSPITALS][50];
    int  hospital_count;
    int  is_available;       /* 1 = available, 0 = not */
    DonationStack history;   /* personal donation history */
    struct Donor *next;
} Donor;

/* Linked List for donors */
typedef struct {
    Donor *head;
    int   count;
    int   next_id;
} DonorList;

/* Patient request node (used in Queue) */
typedef struct PatientRequest {
    int  req_id;
    char patient_name[50];
    char blood_group[5];
    char hospital[50];
    char area[50];
    char request_date[20];
    struct PatientRequest *next;
} PatientRequest;

/* Queue for patient requests */
typedef struct {
    PatientRequest *front;
    PatientRequest *rear;
    int size;
    int next_id;
} RequestQueue;


/* ============================================================
   HELPER: Get today's date as string
   ============================================================ */
void get_today(char *buf) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, 20, "%Y-%m-%d", tm_info);
}

/* Days between two date strings "YYYY-MM-DD" */
int days_between(const char *past, const char *now) {
    struct tm t1 = {0}, t2 = {0};
    sscanf(past, "%d-%d-%d", &t1.tm_year, &t1.tm_mon, &t1.tm_mday);
    sscanf(now,  "%d-%d-%d", &t2.tm_year, &t2.tm_mon, &t2.tm_mday);
    t1.tm_year -= 1900; t1.tm_mon -= 1;
    t2.tm_year -= 1900; t2.tm_mon -= 1;
    time_t time1 = mktime(&t1);
    time_t time2 = mktime(&t2);
    return (int)((time2 - time1) / 86400);
}


/* ============================================================
   STACK OPERATIONS (Donation History)
   ============================================================ */
void stack_init(DonationStack *s) {
    s->top  = NULL;
    s->size = 0;
}

void stack_push(DonationStack *s, const char *date, const char *hospital) {
    DonationRecord *rec = (DonationRecord *)malloc(sizeof(DonationRecord));
    if (!rec) { printf("Memory error!\n"); return; }
    strncpy(rec->date,     date,     19);
    strncpy(rec->hospital, hospital, 49);
    rec->next = s->top;
    s->top    = rec;
    s->size++;
}

DonationRecord *stack_peek(DonationStack *s) {
    return s->top;
}

void stack_print(DonationStack *s) {
    if (!s->top) { printf("  (no history)\n"); return; }
    DonationRecord *cur = s->top;
    int i = 1;
    while (cur) {
        printf("  %d. Date: %s | Hospital: %s\n", i++, cur->date, cur->hospital);
        cur = cur->next;
    }
}

void stack_free(DonationStack *s) {
    DonationRecord *cur = s->top;
    while (cur) {
        DonationRecord *tmp = cur;
        cur = cur->next;
        free(tmp);
    }
    s->top  = NULL;
    s->size = 0;
}


/* ============================================================
   LINKED LIST OPERATIONS (Donor Database)
   ============================================================ */
void list_init(DonorList *dl) {
    dl->head    = NULL;
    dl->count   = 0;
    dl->next_id = 1;
}

Donor *donor_create(DonorList *dl, const char *name, const char *blood_group,
                    const char *area) {
    Donor *d = (Donor *)malloc(sizeof(Donor));
    if (!d) { printf("Memory error!\n"); return NULL; }
    d->id = dl->next_id++;
    strncpy(d->name,        name,        49);
    strncpy(d->blood_group, blood_group, 4);
    strncpy(d->area,        area,        49);
    d->hospital_count = 0;
    d->is_available   = 1;
    stack_init(&d->history);
    d->next = NULL;
    return d;
}

void donor_add_hospital(Donor *d, const char *hospital) {
    if (d->hospital_count >= MAX_HOSPITALS) {
        printf("Max %d hospitals allowed.\n", MAX_HOSPITALS);
        return;
    }
    strncpy(d->hospitals[d->hospital_count++], hospital, 49);
}

void list_add(DonorList *dl, Donor *d) {
    if (!dl->head) {
        dl->head = d;
    } else {
        Donor *cur = dl->head;
        while (cur->next) cur = cur->next;
        cur->next = d;
    }
    dl->count++;
}

Donor *list_find_by_id(DonorList *dl, int id) {
    Donor *cur = dl->head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

void list_delete(DonorList *dl, int id) {
    Donor *cur  = dl->head;
    Donor *prev = NULL;
    while (cur) {
        if (cur->id == id) {
            if (prev) prev->next = cur->next;
            else       dl->head  = cur->next;
            stack_free(&cur->history);
            free(cur);
            dl->count--;
            printf("Donor #%d deleted.\n", id);
            return;
        }
        prev = cur;
        cur  = cur->next;
    }
    printf("Donor #%d not found.\n", id);
}

void list_print_all(DonorList *dl) {
    if (!dl->head) { printf("No donors registered.\n"); return; }
    Donor *cur = dl->head;
    printf("\n%-4s %-20s %-6s %-15s %-12s %s\n",
           "ID", "Name", "Blood", "Area", "Available", "Hospitals");
    printf("---------------------------------------------------------------\n");
    while (cur) {
        printf("%-4d %-20s %-6s %-15s %-12s",
               cur->id, cur->name, cur->blood_group, cur->area,
               cur->is_available ? "Yes" : "No");
        for (int i = 0; i < cur->hospital_count; i++) {
            if (i) printf(", ");
            printf("%s", cur->hospitals[i]);
        }
        printf("\n");
        cur = cur->next;
    }
}

/* Search: all filters optional (pass "" to skip) */
void list_search(DonorList *dl, const char *blood_group,
                 const char *area, const char *hospital,
                 int check_available) {
    Donor *cur = dl->head;
    int   found = 0;
    printf("\n--- Search Results ---\n");
    while (cur) {
        /* Filter: blood group */
        if (strlen(blood_group) && strcasecmp(cur->blood_group, blood_group)) {
            cur = cur->next; continue;
        }
        /* Filter: area */
        if (strlen(area) && strcasecmp(cur->area, area)) {
            cur = cur->next; continue;
        }
        /* Filter: hospital */
        if (strlen(hospital)) {
            int has = 0;
            for (int i = 0; i < cur->hospital_count; i++)
                if (strcasecmp(cur->hospitals[i], hospital) == 0) { has = 1; break; }
            if (!has) { cur = cur->next; continue; }
        }
        /* Filter: available only */
        if (check_available && !cur->is_available) {
            cur = cur->next; continue;
        }
        printf("ID: %d | Name: %-20s | Blood: %-4s | Area: %-15s | Available: %s\n",
               cur->id, cur->name, cur->blood_group, cur->area,
               cur->is_available ? "Yes" : "No");
        found++;
        cur = cur->next;
    }
    if (!found) printf("No donors found matching the criteria.\n");
    else        printf("Total found: %d\n", found);
}

void list_free(DonorList *dl) {
    Donor *cur = dl->head;
    while (cur) {
        Donor *tmp = cur;
        cur = cur->next;
        stack_free(&tmp->history);
        free(tmp);
    }
    dl->head  = NULL;
    dl->count = 0;
}


/* ============================================================
   QUEUE OPERATIONS (Patient Requests)
   ============================================================ */
void queue_init(RequestQueue *q) {
    q->front   = NULL;
    q->rear    = NULL;
    q->size    = 0;
    q->next_id = 1;
}

void queue_enqueue(RequestQueue *q, const char *patient_name,
                   const char *blood_group, const char *hospital,
                   const char *area) {
    PatientRequest *req = (PatientRequest *)malloc(sizeof(PatientRequest));
    if (!req) { printf("Memory error!\n"); return; }
    req->req_id = q->next_id++;
    strncpy(req->patient_name, patient_name, 49);
    strncpy(req->blood_group,  blood_group,  4);
    strncpy(req->hospital,     hospital,     49);
    strncpy(req->area,         area,         49);
    get_today(req->request_date);
    req->next = NULL;

    if (!q->rear) {
        q->front = q->rear = req;
    } else {
        q->rear->next = req;
        q->rear       = req;
    }
    q->size++;
    printf("Request #%d added to queue.\n", req->req_id);
}

PatientRequest *queue_dequeue(RequestQueue *q) {
    if (!q->front) { printf("Queue is empty.\n"); return NULL; }
    PatientRequest *req = q->front;
    q->front = q->front->next;
    if (!q->front) q->rear = NULL;
    q->size--;
    req->next = NULL;
    return req;
}

void queue_print(RequestQueue *q) {
    if (!q->front) { printf("No pending requests.\n"); return; }
    PatientRequest *cur = q->front;
    printf("\n%-6s %-20s %-6s %-20s %-15s %s\n",
           "ReqID", "Patient", "Blood", "Hospital", "Area", "Date");
    printf("------------------------------------------------------------------\n");
    while (cur) {
        printf("%-6d %-20s %-6s %-20s %-15s %s\n",
               cur->req_id, cur->patient_name, cur->blood_group,
               cur->hospital, cur->area, cur->request_date);
        cur = cur->next;
    }
    printf("Total pending: %d\n", q->size);
}

void queue_free(RequestQueue *q) {
    PatientRequest *cur = q->front;
    while (cur) {
        PatientRequest *tmp = cur;
        cur = cur->next;
        free(tmp);
    }
    q->front = q->rear = NULL;
    q->size  = 0;
}


/* ============================================================
   COOLDOWN CHECK
   ============================================================ */
int donor_can_donate(Donor *d) {
    DonationRecord *last = stack_peek(&d->history);
    if (!last) return 1;   /* never donated before */
    char today[20];
    get_today(today);
    int diff = days_between(last->date, today);
    /* Auto-restore availability after cooldown */
    if (diff >= COOLDOWN_DAYS && !d->is_available) {
        d->is_available = 1;
        printf("[System] Donor %s is now available again (cooldown passed).\n", d->name);
    }
    return diff >= COOLDOWN_DAYS;
}


/* ============================================================
   INPUT HELPERS
   ============================================================ */
void clear_input() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void read_line(const char *prompt, char *buf, int size) {
    printf("%s", prompt);
    fgets(buf, size, stdin);
    buf[strcspn(buf, "\n")] = '\0';
}

int read_int(const char *prompt) {
    int val;
    printf("%s", prompt);
    scanf("%d", &val);
    clear_input();
    return val;
}


/* ============================================================
   MENU FUNCTIONS
   ============================================================ */

void menu_add_donor(DonorList *dl) {
    char name[50], blood[5], area[50], hospital[50];
    printf("\n--- Add New Donor ---\n");
    read_line("Name        : ", name,  50);

    do {
        read_line("Blood Group (A+/A-/B+/B-/O+/O-/AB+/AB-): ", blood, 5);
        if (!is_valid_blood_group(blood))
            printf("  Invalid blood group! Please enter one of: A+,A-,B+,B-,O+,O-,AB+,AB-\n");
    } while (!is_valid_blood_group(blood));

    read_line("Area        : ", area,  50);

    Donor *d = donor_create(dl, name, blood, area);
    if (!d) return;

    int n = read_int("Number of hospitals (max 5): ");
    if (n > MAX_HOSPITALS) n = MAX_HOSPITALS;
    for (int i = 0; i < n; i++) {
        char prompt[40];
        sprintf(prompt, "Hospital %d: ", i + 1);
        read_line(prompt, hospital, 50);
        donor_add_hospital(d, hospital);
    }
    list_add(dl, d);
    printf("Donor added! ID: %d\n", d->id);
}

void menu_update_donor(DonorList *dl) {
    int id = read_int("\nEnter Donor ID to update: ");
    Donor *d = list_find_by_id(dl, id);
    if (!d) { printf("Donor not found.\n"); return; }

    printf("1. Update area\n2. Toggle availability\n3. Add hospital\n");
    int ch = read_int("Choice: ");
    if (ch == 1) {
        read_line("New area: ", d->area, 50);
        printf("Area updated.\n");
    } else if (ch == 2) {
        d->is_available = !d->is_available;
        printf("Availability set to: %s\n", d->is_available ? "Available" : "Not Available");
    } else if (ch == 3) {
        char h[50];
        read_line("Hospital name: ", h, 50);
        donor_add_hospital(d, h);
        printf("Hospital added.\n");
    }
}

void menu_donate(DonorList *dl) {
    int id = read_int("\nEnter Donor ID: ");
    Donor *d = list_find_by_id(dl, id);
    if (!d) { printf("Donor not found.\n"); return; }

    if (!donor_can_donate(d)) {
        DonationRecord *last = stack_peek(&d->history);
        printf("Cannot donate yet! Last donation: %s\n", last->date);
        char today2[20]; get_today(today2);
        printf("Must wait 90 days. Please try after %d more days.\n",
               COOLDOWN_DAYS - days_between(last->date, today2));
        return;
    }

    char hospital[50], date[20];
    read_line("Hospital name: ", hospital, 50);
    get_today(date);
    stack_push(&d->history, date, hospital);
    d->is_available = 0;   /* temporarily unavailable after donation */
    printf("Donation recorded on %s at %s.\n", date, hospital);
    printf("Donor marked as unavailable for 90 days.\n");
}

void menu_view_history(DonorList *dl) {
    int id = read_int("\nEnter Donor ID: ");
    Donor *d = list_find_by_id(dl, id);
    if (!d) { printf("Donor not found.\n"); return; }
    printf("\nDonation History for %s (ID: %d):\n", d->name, d->id);
    stack_print(&d->history);
}

void menu_search(DonorList *dl) {
    printf("\n--- Search Donors ---\n");
    printf("(Press Enter to skip a filter)\n");
    char blood[5], area[50], hospital[50];
    read_line("Blood Group : ", blood,    5);
    read_line("Area        : ", area,     50);
    read_line("Hospital    : ", hospital, 50);
    int avail = read_int("Available only? (1=Yes, 0=No): ");
    list_search(dl, blood, area, hospital, avail);
}

void menu_add_request(RequestQueue *q) {
    printf("\n--- Add Patient Request ---\n");
    char name[50], blood[5], hospital[50], area[50];
    read_line("Patient Name : ", name,     50);
    do {
        read_line("Blood Group  (A+/A-/B+/B-/O+/O-/AB+/AB-): ", blood, 5);
        if (!is_valid_blood_group(blood))
            printf("  Invalid blood group!\n");
    } while (!is_valid_blood_group(blood));
    read_line("Hospital     : ", hospital, 50);
    read_line("Area         : ", area,     50);
    queue_enqueue(q, name, blood, hospital, area);
}

void menu_process_request(RequestQueue *q, DonorList *dl) {
    printf("\n--- Process Next Request ---\n");
    PatientRequest *req = queue_dequeue(q);
    if (!req) return;
    printf("Processing request for: %s | Blood: %s | Hospital: %s\n",
           req->patient_name, req->blood_group, req->hospital);
    printf("\nSearching matching donors...\n");
    list_search(dl, req->blood_group, "", req->hospital, 1);
    free(req);
}


/* ============================================================
   SAVE / LOAD  (plain-text .dat file)
   ============================================================ */

/* File format:
   DONORS <count> <next_id>
   DONOR <id> <is_available> <hospital_count> <history_size>
   <name>
   <blood_group>
   <area>
   <hospital_1> ... <hospital_n>
   HISTORY
   <date> | <hospital>   (one per line, newest first)
   REQUESTS <count> <next_id>
   REQUEST <req_id>
   <patient_name>
   <blood_group>
   <hospital>
   <area>
   <request_date>
*/

void save_data(DonorList *dl, RequestQueue *q) {
    FILE *fp = fopen(SAVE_FILE, "w");
    if (!fp) { printf("Error: Cannot open file for saving.\n"); return; }

    /* --- Donors --- */
    fprintf(fp, "DONORS %d %d\n", dl->count, dl->next_id);
    Donor *d = dl->head;
    while (d) {
        fprintf(fp, "DONOR %d %d %d %d\n",
                d->id, d->is_available, d->hospital_count, d->history.size);
        fprintf(fp, "%s\n%s\n%s\n", d->name, d->blood_group, d->area);
        for (int i = 0; i < d->hospital_count; i++)
            fprintf(fp, "%s\n", d->hospitals[i]);
        fprintf(fp, "HISTORY\n");
        /* Write history newest→oldest (stack order) */
        DonationRecord *rec = d->history.top;
        while (rec) {
            fprintf(fp, "%s|%s\n", rec->date, rec->hospital);
            rec = rec->next;
        }
        d = d->next;
    }

    /* --- Requests --- */
    fprintf(fp, "REQUESTS %d %d\n", q->size, q->next_id);
    PatientRequest *r = q->front;
    while (r) {
        fprintf(fp, "REQUEST %d\n%s\n%s\n%s\n%s\n%s\n",
                r->req_id, r->patient_name, r->blood_group,
                r->hospital, r->area, r->request_date);
        r = r->next;
    }

    fclose(fp);
    printf("Data saved to '%s'.\n", SAVE_FILE);
}

void load_data(DonorList *dl, RequestQueue *q) {
    FILE *fp = fopen(SAVE_FILE, "r");
    if (!fp) { printf("No save file found. Starting fresh.\n"); return; }

    int count, next_id;
    if (fscanf(fp, "DONORS %d %d\n", &count, &next_id) != 2) { fclose(fp); return; }
    dl->next_id = next_id;

    for (int i = 0; i < count; i++) {
        int id, avail, hcount, hsize;
        if (fscanf(fp, "DONOR %d %d %d %d\n", &id, &avail, &hcount, &hsize) != 4) break;

        char name[50], blood[5], area[50];
        fgets(name,  50, fp); name[strcspn(name, "\n")]  = '\0';
        fgets(blood,  5, fp); blood[strcspn(blood, "\n")] = '\0';
        fgets(area,  50, fp); area[strcspn(area, "\n")]  = '\0';

        Donor *d = donor_create(dl, name, blood, area);
        if (!d) break;
        d->id           = id;
        d->is_available = avail;

        for (int h = 0; h < hcount; h++) {
            char hosp[50];
            fgets(hosp, 50, fp); hosp[strcspn(hosp, "\n")] = '\0';
            donor_add_hospital(d, hosp);
        }

        /* Read HISTORY header */
        char line[80];
        fgets(line, 80, fp); /* "HISTORY\n" */

        /* Read history records; they are stored newest→oldest.
           To keep stack in same order, collect then push in reverse. */
        char dates[200][20], hosps[200][50];
        int rcount = 0;
        while (rcount < hsize) {
            fgets(line, 80, fp);
            line[strcspn(line, "\n")] = '\0';
            char *sep = strchr(line, '|');
            if (!sep) break;
            *sep = '\0';
            strncpy(dates[rcount], line, 19);
            strncpy(hosps[rcount], sep + 1, 49);
            rcount++;
        }
        /* Push oldest first so top = newest */
        for (int r = rcount - 1; r >= 0; r--)
            stack_push(&d->history, dates[r], hosps[r]);

        list_add(dl, d);
    }
    /* Fix IDs: donor_create auto-increments id using next_id,
       but we set d->id manually, so restore next_id properly. */
    dl->next_id = next_id;

    /* --- Requests --- */
    int rcount2, rnext;
    if (fscanf(fp, "REQUESTS %d %d\n", &rcount2, &rnext) != 2) { fclose(fp); return; }
    q->next_id = rnext;

    for (int i = 0; i < rcount2; i++) {
        int rid;
        if (fscanf(fp, "REQUEST %d\n", &rid) != 1) break;
        char pname[50], blood[5], hosp[50], area[50], date[20];
        fgets(pname, 50, fp); pname[strcspn(pname,"\n")] = '\0';
        fgets(blood,  5, fp); blood[strcspn(blood, "\n")] = '\0';
        fgets(hosp,  50, fp); hosp[strcspn(hosp,  "\n")] = '\0';
        fgets(area,  50, fp); area[strcspn(area,  "\n")] = '\0';
        fgets(date,  20, fp); date[strcspn(date,  "\n")] = '\0';
        queue_enqueue(q, pname, blood, hosp, area);
        /* Fix the date and id since enqueue sets them automatically */
        q->rear->req_id = rid;
        strncpy(q->rear->request_date, date, 19);
    }
    q->next_id = rnext;

    fclose(fp);
    printf("Loaded %d donor(s) and %d request(s) from '%s'.\n",
           dl->count, q->size, SAVE_FILE);
}


/* ============================================================
   STATISTICS
   ============================================================ */
void menu_statistics(DonorList *dl) {
    printf("\n========== STATISTICS ==========\n");
    printf("Total Donors    : %d\n", dl->count);

    int avail = 0;
    int bg_count[8] = {0};
    const char *bg_names[] = {"A+","A-","B+","B-","O+","O-","AB+","AB-"};
    int total_donations = 0;

    Donor *cur = dl->head;
    while (cur) {
        if (cur->is_available) avail++;
        for (int i = 0; i < 8; i++)
            if (strcasecmp(cur->blood_group, bg_names[i]) == 0) { bg_count[i]++; break; }
        total_donations += cur->history.size;
        cur = cur->next;
    }

    printf("Available Now   : %d\n", avail);
    printf("Unavailable     : %d\n", dl->count - avail);
    printf("Total Donations : %d\n\n", total_donations);
    printf("Blood Group Breakdown:\n");
    for (int i = 0; i < 8; i++)
        if (bg_count[i] > 0)
            printf("  %-4s : %d donor(s)\n", bg_names[i], bg_count[i]);
    printf("================================\n");
}


/* ============================================================
   MAIN
   ============================================================ */
int main() {
    DonorList    donors;
    RequestQueue requests;
    list_init(&donors);
    queue_init(&requests);

    /* --- Load saved data (if any) --- */
    load_data(&donors, &requests);

    /* --- Sample data only if no donors loaded --- */
    if (donors.count == 0) {
        Donor *d1 = donor_create(&donors, "Rahim Uddin",  "O+",  "Mirpur");
        donor_add_hospital(d1, "DMCH"); donor_add_hospital(d1, "Square Hospital");
        list_add(&donors, d1);

        Donor *d2 = donor_create(&donors, "Karim Hossain", "A+",  "Dhanmondi");
        donor_add_hospital(d2, "Square Hospital"); donor_add_hospital(d2, "Popular Hospital");
        list_add(&donors, d2);
        stack_push(&d2->history, "2024-10-15", "Square Hospital");
        d2->is_available = 0;

        Donor *d3 = donor_create(&donors, "Sumaiya Akter", "B+",  "Uttara");
        donor_add_hospital(d3, "Evercare"); donor_add_hospital(d3, "DMCH");
        list_add(&donors, d3);

        Donor *d4 = donor_create(&donors, "Nayeem Islam",  "AB-", "Mirpur");
        donor_add_hospital(d4, "DMCH");
        list_add(&donors, d4);
        printf("Sample data loaded.\n");
    }

    int choice;
    do {
        printf("\n========================================\n");
        printf("   BLOOD DONATION MANAGEMENT SYSTEM\n");
        printf("========================================\n");
        printf(" 1. View All Donors\n");
        printf(" 2. Add New Donor\n");
        printf(" 3. Update Donor Info\n");
        printf(" 4. Delete Donor\n");
        printf(" 5. Search Donors\n");
        printf(" 6. Record Blood Donation\n");
        printf(" 7. View Donation History\n");
        printf(" 8. Add Patient Request\n");
        printf(" 9. View All Requests (Queue)\n");
        printf("10. Process Next Request\n");
        printf("11. Statistics\n");
        printf("12. Save Data\n");
        printf(" 0. Exit\n");
        printf("========================================\n");
        choice = read_int("Enter choice: ");

        switch (choice) {
            case 1:  list_print_all(&donors);              break;
            case 2:  menu_add_donor(&donors);              break;
            case 3:  menu_update_donor(&donors);           break;
            case 4: {
                int id = read_int("Enter Donor ID to delete: ");
                list_delete(&donors, id);
                break;
            }
            case 5:  menu_search(&donors);                 break;
            case 6:  menu_donate(&donors);                 break;
            case 7:  menu_view_history(&donors);           break;
            case 8:  menu_add_request(&requests);          break;
            case 9:  queue_print(&requests);               break;
            case 10: menu_process_request(&requests, &donors); break;
            case 11: menu_statistics(&donors);             break;
            case 12: save_data(&donors, &requests);        break;
            case 0: {
                int s = read_int("Save before exit? (1=Yes, 0=No): ");
                if (s) save_data(&donors, &requests);
                printf("Goodbye!\n");
                break;
            }
            default: printf("Invalid choice.\n");
        }
    } while (choice != 0);

    list_free(&donors);
    queue_free(&requests);
    return 0;
}
