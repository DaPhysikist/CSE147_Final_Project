This app is a modified version of Qorvo's Nearby Interaction Background iOS App. It allows Qorvo Ultrawideband (UWB) devices nearby to the phone to detect the phone's presence over UWB if the app is open in the background (does not matter if phone is locked, so long as app has not been cleared from background applications list). The app has been modified to remove all references to Qorvo branding, which has been replaced with ShutEye branding instead. In addition, additional modifications have been made to allow for retrieving analytics data.

### Directory Breakdown:
- ShutEye App - The only folder with relevant code.
    - ChartView/ChartView.swift
        - Contains all the code used for fetching data from the data_server and displaying it in three neat charts in the app. Provides exactly the same functionality as the web app.
    - AppViewController.swift
        - Modified to include a button used to switch to the ChartView so that the user can retrieve analytics data.