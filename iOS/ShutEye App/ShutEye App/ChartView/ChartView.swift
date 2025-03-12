import SwiftUI
import Charts

struct BarChartView: View {
    @Environment(\.presentationMode) var presentationMode  // Back button support
    @State private var appliances: [String] = []
    @State private var selectedAppliance: String? = nil
    @State private var dates: [String] = []
    @State private var selectedDate: String? = nil
    @State private var chartData: [PeriodicData] = []
    
    var body: some View {
        VStack {
            Text("Energy Consumption")
                .font(.title2)
                .bold()
                .multilineTextAlignment(.center)
                .padding()
            
            // Appliance Picker
            Picker("Select Appliance", selection: $selectedAppliance) {
                Text("Select Appliance").tag(String?.none)
                ForEach(appliances, id: \.self) { appliance in
                    Text(appliance).tag(String?.some(appliance))
                }
            }
            .pickerStyle(MenuPickerStyle())
            .onChange(of: selectedAppliance) { _ in
                loadAvailableDates()
            }
            .padding()
            
            // Date Picker (Enabled only if an appliance is selected)
            if let selectedAppliance = selectedAppliance {
                Picker("Select Date", selection: $selectedDate) {
                    Text("Select Date").tag(String?.none)
                    ForEach(dates, id: \.self) { date in
                        Text(date).tag(String?.some(date))
                    }
                }
                .pickerStyle(MenuPickerStyle())
                .padding()
            }
            
            // Fetch Data Button (Enabled only if both appliance & date are selected)
            Button("Fetch Data") {
                fetchData()
            }
            .buttonStyle(.borderedProminent)
            .disabled(selectedAppliance == nil || selectedDate == nil)
            .padding()

            // Chart Display
            ScrollView {
                VStack {
                    if chartData.isEmpty {
                        Text("Select appliance and date to load data")
                            .font(.subheadline)
                            .foregroundColor(.gray)
                            .padding()
                    } else {
                        Text("Total Energy Consumption (Wh)").font(.headline)
                        Chart(hourlyAggregatedData, id: \.hour) { data in
                            BarMark(
                                x: .value("Time", data.hour),
                                y: .value("Power (Wh)", data.totalEnergy)
                            )
                        }
                        .chartXAxis {
                            AxisMarks(position: .bottom) { value in
                                AxisValueLabel(centered: true) {
                                    Text(value.as(String.self) ?? "")
                                        .rotationEffect(Angle(degrees: -90))
                                        .frame(width: 60, height: 50)
                                        .contentShape(Rectangle())
                                }
                                AxisGridLine()
                            }
                        }
                        .chartYAxis {
                            AxisMarks(position: .leading)
                        }
                        .frame(height: 280)
                        .frame(maxWidth: .infinity)
                        .padding(.horizontal, 32)
                        .padding(.bottom, 120) // Increased bottom padding

                        // Chart 2: Energy by User Presence
                        Text("Energy by User Presence")
                            .font(.headline)
                        Chart {
                            ForEach(hourlyAggregatedData, id: \.hour) { data in
                                BarMark(
                                    x: .value("Time", data.hour),
                                    y: .value("Power (Wh)", data.userPresenceEnergy)
                                )
                                .foregroundStyle(by: .value("Presence", "With User"))

                                BarMark(
                                    x: .value("Time", data.hour),
                                    y: .value("Power (Wh)", data.noUserPresenceEnergy)
                                )
                                .foregroundStyle(by: .value("Presence", "Without User"))
                            }
                        }
                        .chartXAxis {
                            AxisMarks(position: .bottom) { value in
                                AxisValueLabel(centered: true) {
                                    Text(value.as(String.self) ?? "")
                                        .rotationEffect(Angle(degrees: -90))
                                        .frame(width: 60, height: 50)
                                        .contentShape(Rectangle())
                                }
                                AxisGridLine()
                            }
                        }
                        .chartYAxis {
                            AxisMarks(position: .leading)
                        }
                        .chartLegend(position: .top, alignment: .center)
                        .frame(height: 280)
                        .frame(maxWidth: .infinity)
                        .padding(.horizontal, 32)
                        .padding(.bottom, 120) // Increased bottom padding
                        
                        // Chart 3: User Presence Percentage per Hour
                        Text("User Presence Percentage per Hour")
                            .font(.headline)
                        Chart(hourlyAggregatedData, id: \.hour) { data in
                            BarMark(
                                x: .value("Time", data.hour),
                                y: .value("Presence (%)", data.userPresencePercentage)
                            )
                        }
                        .chartXAxis {
                            AxisMarks(position: .bottom) { value in
                                AxisValueLabel(centered: true) {
                                    Text(value.as(String.self) ?? "")
                                        .rotationEffect(Angle(degrees: -90))
                                        .frame(width: 60, height: 50)
                                        .contentShape(Rectangle())
                                }
                                AxisGridLine()
                            }
                        }
                        .chartYAxis {
                            AxisMarks(position: .leading)
                        }
                        .frame(height: 280)
                        .frame(maxWidth: .infinity)
                        .padding(.horizontal, 32)
                        .padding(.bottom, 120) // Increased bottom padding
                    }
                }
            }

            // Back Button
            Button("Back") {
                presentationMode.wrappedValue.dismiss()  // Dismiss view and go back
            }
            .buttonStyle(.bordered)
            .padding()
        }
        .onAppear {
            loadApplianceNames()
        }
    }
    
    struct HourlyAggregatedData {
        let hour: String
        let totalEnergy: Double
        let userPresenceEnergy: Double
        let noUserPresenceEnergy: Double
        let userPresencePercentage: Double
    }
    
    var hourlyAggregatedData: [HourlyAggregatedData] {
        let groupedData = Dictionary(grouping: chartData) { formatHour(from: $0.local_time) }

        return groupedData.map { (hour, entries) in
            var totalMilliwattSeconds = 0.0
            var userPresenceMilliwattSeconds = 0.0
            var noUserPresenceMilliwattSeconds = 0.0
            var totalPresenceTime = 0.0
            var totalTimeInHour = 0.0

            // Sort entries by time to avoid incorrect calculations
            let sortedEntries = entries.sorted {
                dateFromString($0.local_time) < dateFromString($1.local_time)
            }

            if sortedEntries.count > 1 {
                for i in 1..<sortedEntries.count {
                    let prev = sortedEntries[i - 1]
                    let curr = sortedEntries[i]

                    let prevTime = dateFromString(prev.local_time)
                    let currTime = dateFromString(curr.local_time)

                    let timeDiff = max(currTime.timeIntervalSince(prevTime), 0)

                    let milliWattSeconds = timeDiff * Double(prev.current_power)

                    totalMilliwattSeconds += milliWattSeconds
                    totalTimeInHour += timeDiff

                    if prev.user_presence_detected {
                        userPresenceMilliwattSeconds += milliWattSeconds
                        totalPresenceTime += timeDiff
                    } else {
                        noUserPresenceMilliwattSeconds += milliWattSeconds
                    }
                }
            }

            let totalWattHours = totalMilliwattSeconds / 3_600_000
            let userPresenceWattHours = userPresenceMilliwattSeconds / 3_600_000
            let noUserPresenceWattHours = noUserPresenceMilliwattSeconds / 3_600_000
            let presencePercentage = totalTimeInHour > 0 ? (totalPresenceTime / totalTimeInHour) * 100 : 0

            return HourlyAggregatedData(
                hour: hour,
                totalEnergy: totalWattHours,
                userPresenceEnergy: userPresenceWattHours,
                noUserPresenceEnergy: noUserPresenceWattHours,
                userPresencePercentage: presencePercentage
            )
        }
        .sorted { $0.hour < $1.hour }
    }

    func formatHour(from dateString: String) -> String {
        let inputFormatter = DateFormatter()
        inputFormatter.dateFormat = "yyyy-MM-dd HH:mm:ss"

        let outputFormatter = DateFormatter()
        outputFormatter.dateFormat = "HH:00"

        if let date = inputFormatter.date(from: dateString) {
            return outputFormatter.string(from: date)
        }
        return dateString
    }
    
    func dateFromString(_ dateString: String) -> Date {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
        return formatter.date(from: dateString) ?? Date()
    }

    func loadApplianceNames() {
        guard let url = URL(string: "http://192.168.8.27:6543/appliance_names") else {
            print("Invalid URL")
            return
        }

        URLSession.shared.dataTask(with: url) { data, response, error in
            if let error = error {
                print("Error fetching appliance names: \(error)")
                return
            }

            guard let data = data else {
                print("No data received from server")
                return
            }

            do {
                // Decode JSON as a dictionary containing "appliance_names" key
                let json = try JSONSerialization.jsonObject(with: data, options: []) as? [String: Any]
                
                if let applianceNames = json?["appliance_names"] as? [String] {
                    DispatchQueue.main.async {
                        self.appliances = applianceNames
                    }
                } else {
                    print("Error: JSON format does not match expected structure")
                }
            } catch {
                print("Error parsing JSON: \(error)")
            }
        }.resume()
    }

    func loadAvailableDates() {
        guard let selectedAppliance = selectedAppliance else { return }
        let encodedName = selectedAppliance.addingPercentEncoding(withAllowedCharacters: .urlPathAllowed) ?? ""
        guard let url = URL(string: "http://192.168.8.27:6543/available_dates/\(encodedName)") else {
            print("Invalid URL")
            return
        }

        URLSession.shared.dataTask(with: url) { data, response, error in
            if let error = error {
                print("Error fetching available dates: \(error)")
                return
            }

            guard let data = data else {
                print("No data received from server")
                return
            }

            do {
                // Decode JSON as a dictionary containing "dates" key
                let json = try JSONSerialization.jsonObject(with: data, options: []) as? [String: Any]
                
                if let dateList = json?["dates"] as? [String] {
                    DispatchQueue.main.async {
                        self.dates = dateList
                    }
                } else {
                    print("Error: JSON format does not match expected structure")
                }
            } catch {
                print("Error parsing JSON: \(error)")
            }
        }.resume()
    }

    func fetchData() {
        guard let selectedAppliance = selectedAppliance, let selectedDate = selectedDate else { return }
        let encodedName = selectedAppliance.addingPercentEncoding(withAllowedCharacters: .urlPathAllowed) ?? ""
        guard let url = URL(string: "http://192.168.8.27:6543/shuteye_periodic_measurement_data/\(encodedName)") else { return }

        URLSession.shared.dataTask(with: url) { data, _, _ in
            if let data = data, let decoded = try? JSONDecoder().decode([String: PeriodicData].self, from: data) {
                DispatchQueue.main.async {
                    self.chartData = decoded.map { $0.value }.filter { $0.local_time.contains(selectedDate) }
                }
            }
        }.resume()
    }
}

struct PeriodicData: Codable {
    var appliance_name: String
    var local_time: String
    var current_power: Int
    var distance_ultrasonic: Int
    var distance_bluetooth: Int
    var distance_ultrawideband: Int
    var user_presence_detected: Bool

    enum CodingKeys: String, CodingKey {
        case appliance_name
        case local_time
        case current_power
        case distance_ultrasonic
        case distance_bluetooth
        case distance_ultrawideband
        case user_presence_detected
    }

    // Custom decoding to handle user_presence_detected as Int
    init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        appliance_name = try container.decode(String.self, forKey: .appliance_name)
        local_time = try container.decode(String.self, forKey: .local_time)
        current_power = try container.decode(Int.self, forKey: .current_power)
        distance_ultrasonic = try container.decode(Int.self, forKey: .distance_ultrasonic)
        distance_bluetooth = try container.decode(Int.self, forKey: .distance_bluetooth)
        distance_ultrawideband = try container.decode(Int.self, forKey: .distance_ultrawideband)

        let presenceInt = try container.decode(Int.self, forKey: .user_presence_detected)
        user_presence_detected = presenceInt != 0
    }
}

