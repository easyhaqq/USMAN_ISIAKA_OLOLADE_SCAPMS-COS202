// Console tests for the SCAPMS core (no GUI needed). Run: ./scapms_tests
#include "controller/ClinicManager.h"
#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/DateUtils.h"
#include "utils/Exceptions.h"

#include <cstdio>
#include <ctime>
#include <iostream>
#include <random>

using namespace scapms;

static int failures = 0;
static int checks = 0;

#define CHECK(condition)                                                              \
    do {                                                                              \
        ++checks;                                                                     \
        if (!(condition)) {                                                           \
            ++failures;                                                               \
            std::cerr << "FAILED line " << __LINE__ << ": " #condition << '\n';      \
        }                                                                             \
    } while (0)

template <typename Exception, typename Fn>
static bool throwsType(Fn fn) {
    try { fn(); } catch (const Exception&) { return true; } catch (...) { return false; }
    return false;
}

// Date `days` from today, moved forward to a weekday when weekday == true.
static std::string futureDate(int days, bool weekday) {
    std::time_t t = std::time(nullptr) + static_cast<std::time_t>(days) * 86400;
    char buffer[11];
    while (true) {
        std::strftime(buffer, sizeof buffer, "%Y-%m-%d", std::localtime(&t));
        if (!weekday || !utils::date::isWeekend(buffer)) return buffer;
        t += 86400;
    }
}

static void testDates() {
    CHECK(utils::date::isValidDate("2024-02-29"));
    CHECK(!utils::date::isValidDate("2023-02-29"));
    CHECK(!utils::date::isValidDate("2026-13-01"));
    CHECK(utils::date::dayName("2026-11-08") == "Sunday");
    CHECK(utils::date::isValidTime("23:59") && !utils::date::isValidTime("24:00"));
}

static void testSorting() {
    std::mt19937 rng(42);
    for (auto algorithm : controller::allSortAlgorithms()) {
        for (int size : {0, 1, 2, 7, 100, 1000}) {
            std::vector<int> values(static_cast<std::size_t>(size));
            for (auto& v : values) v = static_cast<int>(rng() % 50);
            controller::sortItems(values, algorithm, [](int a, int b) { return a < b; });
            bool ordered = true;
            for (std::size_t i = 1; i < values.size(); ++i) ordered = ordered && values[i - 1] <= values[i];
            CHECK(ordered);
        }
        std::vector<int> sorted(500);
        for (int i = 0; i < 500; ++i) sorted[static_cast<std::size_t>(i)] = i;   // already sorted input
        controller::sortItems(sorted, algorithm, [](int a, int b) { return a < b; });
        CHECK(sorted.front() == 0 && sorted.back() == 499);
    }
    // Stability: merge and insertion keep equal keys in their original order.
    for (auto algorithm : {controller::SortAlgorithm::Merge, controller::SortAlgorithm::Insertion}) {
        std::vector<std::pair<int, int>> items = {{2, 0}, {1, 1}, {2, 2}, {1, 3}, {2, 4}};
        controller::sortItems(items, algorithm, [](auto& a, auto& b) { return a.first < b.first; });
        CHECK(items[0].second == 1 && items[1].second == 3 && items[2].second == 0 && items[4].second == 4);
    }
}

static model::PatientPtr general(const std::string& id, const std::string& name, int age) {
    return std::make_unique<model::GeneralPatient>(id, name, age, "08031234567", "Student");
}

static void testValidation() {
    CHECK(throwsType<utils::InvalidAgeException>([] { general("PAT-0001", "Ada Obi", 130); }));
    CHECK(throwsType<utils::ValidationException>([] { general("P1", "Ada Obi", 20); }));
    CHECK(throwsType<utils::ValidationException>([] { general("PAT-0001", "", 20); }));
    CHECK(throwsType<utils::ValidationException>([] {
        model::GeneralPatient p("PAT-0001", "Ada Obi", 20, "not-a-phone", "Student");
    }));
    CHECK(throwsType<utils::ValidationException>([] {
        model::EmergencyPatient p("PAT-0001", "Ada Obi", 20, "08031234567", 9, "Fever");
    }));
    controller::ClinicManager manager;
    manager.registerPatient(general("PAT-0001", "Ada Obi", 20));
    CHECK(throwsType<utils::DuplicateIdException>([&] { manager.registerPatient(general("PAT-0001", "Ben Udo", 30)); }));
}

static void testSearchAndSort() {
    controller::ClinicManager manager;
    manager.registerPatient(general("PAT-0003", "Chidi Okeke", 40));
    manager.registerPatient(general("PAT-0001", "Amina Bello", 22));
    manager.registerPatient(general("PAT-0002", "Bola Ade", 31));
    manager.registerPatient(std::make_unique<model::EmergencyPatient>("PAT-0004", "Amaka Eze", 19, "08030000000", 3, "Asthma"));

    using controller::SearchAlgorithm;
    using controller::SearchField;
    auto linear = manager.search(SearchField::Id, "2", SearchAlgorithm::Auto);
    CHECK(linear.algorithmUsed == "Linear Search" && linear.matches.size() == 1 && linear.matches[0]->name() == "Bola Ade");
    CHECK(throwsType<utils::SearchException>([&] { manager.search(SearchField::Id, "1", SearchAlgorithm::Binary); }));
    CHECK(throwsType<utils::SearchException>([&] { manager.search(SearchField::Id, "abc", SearchAlgorithm::Auto); }));
    CHECK(throwsType<utils::SearchException>([&] { manager.search(SearchField::Name, "  ", SearchAlgorithm::Auto); }));

    manager.sortPatients(model::SortKey::Id, controller::SortAlgorithm::Quick);
    CHECK(manager.patients()[0]->id() == "PAT-0001" && manager.patients()[3]->id() == "PAT-0004");
    auto binary = manager.search(SearchField::Id, "PAT-0003", SearchAlgorithm::Auto);
    CHECK(binary.algorithmUsed == "Binary Search" && binary.matches.size() == 1);

    auto recursive = manager.search(SearchField::Name, "am", SearchAlgorithm::Auto);
    CHECK(recursive.algorithmUsed == "Recursive Search" && recursive.matches.size() == 2);

    manager.sortPatients(model::SortKey::Name, controller::SortAlgorithm::Merge);
    CHECK(manager.patients()[0]->name() == "Amaka Eze");
    auto prefix = manager.search(SearchField::Name, "AM", SearchAlgorithm::Auto);
    CHECK(prefix.algorithmUsed == "Binary Search" && prefix.matches.size() == 2);

    auto category = manager.search(SearchField::Category, "emer", SearchAlgorithm::Auto);
    CHECK(category.matches.size() == 1 && category.matches[0]->id() == "PAT-0004");

    manager.sortPatients(model::SortKey::Age, controller::SortAlgorithm::Selection);
    CHECK(manager.patients()[0]->age() == 19 && manager.patients()[3]->age() == 40);
}

static void testAppointmentsAndUndo() {
    controller::ClinicManager manager;
    manager.registerPatient(general("PAT-0001", "Amina Bello", 22));
    manager.registerPatient(std::make_unique<model::EmergencyPatient>("PAT-0002", "Musa Ali", 50, "08030000000", 5, "Chest pain"));
    manager.registerPatient(std::make_unique<model::FollowUpPatient>("PAT-0003", "Ngozi Uche", 35, "ngozi@miva.edu.ng",
                                                                     "Malaria", utils::date::today()));
    auto& appointments = manager.appointments();
    const std::string weekday = futureDate(3, true);
    std::string saturday;
    for (int days = 1; saturday.empty() || utils::date::dayOfWeek(saturday) != 6; ++days) {
        saturday = futureDate(days, false);
    }

    CHECK(throwsType<utils::AppointmentException>([&] {
        appointments.schedule("PAT-0001", saturday, "10:00", "General Consultation", "Dr. A. Bello (General Practice)", "");
    }));
    CHECK(throwsType<utils::AppointmentException>([&] {
        appointments.schedule("PAT-0001", "2020-01-06", "10:00", "General Consultation", "Dr. A. Bello (General Practice)", "");
    }));
    CHECK(throwsType<utils::AppointmentException>([&] {
        appointments.schedule("PAT-0003", utils::date::today(), "23:59", "Follow-Up Review", "Dr. A. Bello (General Practice)", "");
    }));
    const std::string a1 = appointments.schedule("PAT-0001", weekday, "10:00", "General Consultation",
                                                 "Dr. A. Bello (General Practice)", "");
    CHECK(throwsType<utils::AppointmentException>([&] {   // same doctor, same slot
        appointments.schedule("PAT-0003", weekday, "10:00", "Follow-Up Review", "Dr. A. Bello (General Practice)", "");
    }));
    const std::string a2 = appointments.schedule("PAT-0002", saturday, "02:00", "Emergency Care", "Dr. C. Okafor (Emergency)", "");
    CHECK(throwsType<utils::AppointmentException>([&] { appointments.cancel(a2); }));   // critical emergency

    appointments.cancel(a1);
    CHECK(manager.database().findAppointment(a1)->status() == model::AppointmentStatus::Cancelled);
    CHECK(manager.undoLastOperation().find("Cancel") == 0);
    CHECK(manager.database().findAppointment(a1)->isPending());

    // Delete + undo restores the patient and their appointments.
    manager.deletePatient("PAT-0001");
    CHECK(!manager.isPatientIdTaken("PAT-0001") && manager.database().findAppointment(a1) == nullptr);
    manager.undoLastOperation();
    CHECK(manager.isPatientIdTaken("PAT-0001") && manager.database().findAppointment(a1) != nullptr);
    CHECK(manager.findPatient("PAT-0001")->hasAppointment(a1));

    // Modify + undo.
    manager.updatePatient(std::make_unique<model::GeneralPatient>("PAT-0001", "Amina B. Bello", 23, "08031234567", "Staff"));
    CHECK(manager.findPatient("PAT-0001")->name() == "Amina B. Bello");
    CHECK(manager.findPatient("PAT-0001")->hasAppointment(a1));
    manager.undoLastOperation();
    CHECK(manager.findPatient("PAT-0001")->name() == "Amina Bello" && manager.findPatient("PAT-0001")->age() == 22);

    // Remove appointment record + undo.
    appointments.remove(a1);
    CHECK(manager.database().findAppointment(a1) == nullptr);
    manager.undoLastOperation();
    CHECK(manager.database().findAppointment(a1) != nullptr);
}

static void testQueueAndCache() {
    controller::ClinicManager manager;
    for (int i = 1; i <= 12; ++i) {
        char id[9];
        std::snprintf(id, sizeof id, "PAT-%04d", i);
        manager.registerPatient(general(id, "Patient Number", 20 + i));
    }
    CHECK(manager.recentCache().size() == controller::RecentPatientCache::kCapacity);
    CHECK(manager.recentCache().at(0)->id() == "PAT-0012");
    manager.findPatient("PAT-0005");
    CHECK(manager.recentCache().at(0)->id() == "PAT-0005");

    manager.addToQueue("PAT-0003");
    manager.addToQueue("PAT-0001");
    CHECK(throwsType<utils::ClinicException>([&] { manager.addToQueue("PAT-0003"); }));
    CHECK(manager.queueRows()[1].estimatedStartMinutes == 15);
    CHECK(manager.callNextPatient().patientId == "PAT-0003");    // first in, first out
    CHECK(manager.callNextPatient().patientId == "PAT-0001");
    CHECK(throwsType<utils::NotFoundException>([&] { manager.callNextPatient(); }));
}

static void testPersistence() {
    controller::ClinicManager manager;
    manager.registerPatient(general("PAT-0001", "Amina Bello", 22));
    manager.registerPatient(std::make_unique<model::EmergencyPatient>("PAT-0002", "Musa Ali", 50, "08030000000", 2, "Sprained ankle"));
    manager.appointments().schedule("PAT-0001", futureDate(2, true), "09:00", "Dental Check", "Dr. F. Musa (Dental)", "Bring card");
    const std::string path = "scapms_test_data.txt";
    manager.save(path);

    controller::ClinicManager loaded;
    CHECK(loaded.load(path).empty());
    CHECK(loaded.patients().size() == 2 && loaded.database().appointments().size() == 1);
    CHECK(loaded.findPatient("PAT-0001")->appointmentIds().size() == 1);
    CHECK(loaded.suggestedPatientId() == "PAT-0003");
    CHECK(throwsType<utils::FileException>([&] { loaded.load("does_not_exist.txt"); }));
    CHECK(loaded.patients().size() == 2);   // failed load leaves data untouched
    std::remove(path.c_str());
}

int main() {
    testDates();
    testSorting();
    testValidation();
    testSearchAndSort();
    testAppointmentsAndUndo();
    testQueueAndCache();
    testPersistence();
    std::cout << (checks - failures) << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
