import SwiftUI

struct PIDSelectionView: View {
    @State private var selectedPIDs: [String] = []
    let supportedPIDs = [
        "Engine RPM (PID 0C)",
        "Vehicle Speed (PID 0D)",
        "Engine Coolant Temperature (PID 05)",
        "Intake Air Temperature (PID 0F)",
        "Mass Air Flow Rate (PID 10)",
        "Throttle Position (PID 11)",
        "Time Since Engine Start (PID 1F)",
        "Absolute Barometric Pressure (PID 33)"
    ]

    var body: some View {
        NavigationView {
            VStack {
                Text("Select Preferred PIDs")
                    .font(.headline)
                    .padding()

                ScrollView {
                    ForEach(supportedPIDs, id: \.self) { pid in
                        HStack {
                            Text(pid)
                            Spacer()
                            if selectedPIDs.contains(pid) {
                                Image(systemName: "checkmark")
                                    .foregroundColor(.blue)
                            }
                        }
                        .padding()
                        .background(Color.gray.opacity(0.1))
                        .cornerRadius(8)
                        .onTapGesture {
                            if let index = selectedPIDs.firstIndex(of: pid) {
                                selectedPIDs.remove(at: index)
                            } else {
                                selectedPIDs.append(pid)
                            }
                        }
                    }
                }
                .padding()

                NavigationLink(destination: PIDCalculatorView()) {
                    Text("Go to PID Calculator")
                        .font(.headline)
                        .foregroundColor(.white)
                        .padding()
                        .background(Color.blue)
                        .cornerRadius(8)
                }
                .padding()
            }
            .navigationTitle("PID Selection")
        }
    }
}

struct PIDCalculatorView: View {
    @State private var engineRPM: Double = 0
    @State private var vehicleSpeed: Double = 0

    var body: some View {
        VStack {
            Text("PID Calculator")
                .font(.headline)
                .padding()

            HStack {
                Text("Engine RPM:")
                TextField("Enter RPM", value: $engineRPM, formatter: NumberFormatter())
                    .textFieldStyle(RoundedBorderTextFieldStyle())
                    .keyboardType(.decimalPad)
            }
            .padding()

            HStack {
                Text("Vehicle Speed:")
                TextField("Enter Speed", value: $vehicleSpeed, formatter: NumberFormatter())
                    .textFieldStyle(RoundedBorderTextFieldStyle())
                    .keyboardType(.decimalPad)
            }
            .padding()

            Text("Example: Engine RPM vs Vehicle Speed")
                .font(.subheadline)
                .padding()

            Spacer()
        }
        .padding()
    }
}

struct PIDSelectionView_Previews: PreviewProvider {
    static var previews: some View {
        PIDSelectionView()
    }
}
