import SwiftUI
import Charts

struct BarChartView: View {
    let applianceName: String  // Receive selected device
    @State private var chartData: [HistoricalData] = []
    
    var body: some View {
        VStack {
            Text("Energy Consumption for \(applianceName)")
                .font(.title)
                .padding()
            
            Chart(chartData, id: \.local_time) { data in
                BarMark(
                    x: .value("Date", data.local_time),
                    y: .value("Today Energy", data.today_energy)
                )
            }
            .frame(height: 300)
            .padding()

            Button("Fetch Data") {
                fetchData()
            }
            .buttonStyle(.borderedProminent)
            .padding()
        }
        .onAppear {
            fetchData()
        }
    }
    
    func fetchData() {
        let encodedName = applianceName.addingPercentEncoding(withAllowedCharacters: .urlPathAllowed) ?? ""
        guard let url = URL(string: "http://192.168.8.27:6543/shuteye_historical_data/\(encodedName)") else { return }

        let task = URLSession.shared.dataTask(with: url) { data, response, error in
            if let error = error {
                print("Client error: \(error)")
                return
            }
            guard let httpResponse = response as? HTTPURLResponse,
                  (200...299).contains(httpResponse.statusCode) else {
                print("Server error")
                return
            }

            if let data = data {
                do {
                    let decodedResponse = try JSONDecoder().decode([String: HistoricalData].self, from: data)
                    DispatchQueue.main.async {
                        self.chartData = decodedResponse.map { $0.value }
                    }
                } catch {
                    print("Failed to decode data: \(error)")
                }
            }
        }
        task.resume()
    }
}

struct HistoricalData: Codable {
    var appliance_name: String
    var local_time: String
    var today_runtime: Double
    var month_runtime: Double
    var today_energy: Double
    var month_energy: Double
}
