Option Explicit On
Option Strict On
Imports FTD2XX_NET

Public Class frmMain

    Dim mSerialList As New ArrayList
    Dim mDevice As FTDI
    Dim mConnected As Boolean
    Dim mCh1Range As Integer

    Private Sub frmMain_FormClosed(ByVal sender As Object, ByVal e As System.Windows.Forms.FormClosedEventArgs) Handles Me.FormClosed
        If mConnected = True Then
            disconnect()
        End If
    End Sub

    Private Sub frmMain_Load(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.Load
        btnDisconnect.Visible = False
        btnSendPosition.Visible = False
        btnQuerySensor.Visible = False
        txtPositionCommand.Visible = False
        txtSensorValue.Visible = False
        lblDigitalPosCmd.Visible = False
        lblSensorValue.Visible = False
        lblRangeUnits1.Visible = False
        lblRangeUnits2.Visible = False
        listDevices()
    End Sub

    Private Sub btnListDevices_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnListDevices.Click
        listDevices()
    End Sub

    Private Sub listDevices()
        Try
            lstDeviceList.Items.Clear()
            mSerialList.Clear()
            mDevice = New FTDI

            Dim FTStatus As New FTDI.FT_STATUS
            Dim uDevCount As UInteger
            Dim i As Integer
            Dim aChar As Char
            Dim bChar As Char

            'Determine the number of FTDI devices connected to the machine
            FTStatus = mDevice.GetNumberOfDevices(uDevCount)

            If uDevCount > 0 And FTStatus = FTDI.FT_STATUS.FT_OK Then
                'Create a device list array to hold information
                'Creating an array size based off uDevCount would occasionally give errors so it is hard coded for 20 devices (up to 10 C400 controllers with 2 ports each)
                Dim devList(19) As FTDI.FT_DEVICE_INFO_NODE
                'Populate the device list array
                FTStatus = mDevice.GetDeviceList(devList)
                If FTStatus <> FTDI.FT_STATUS.FT_OK Then
                    Throw New System.Exception("An exception occured when attempting to create a USB device list. FTStatus = " & FTStatus.ToString)
                End If

                'Add devices to the global serial number array list and FTDI device listbox, only serial numbers that end in "A" are added since the FTDI drivers automatically
                'add "A" or "B" to the end of the serial number programmed with the 'FT_PROG" flash program.  Port B is not physically connected
                'to anything, so it is not reported as a device available to connect to.
                For i = 0 To Convert.ToInt32(uDevCount - 1)
                    If devList(i).SerialNumber.Length > 0 Then
                        aChar = devList(i).SerialNumber.Last
                        bChar = devList(i).Description.Last
                        If aChar = "A" And bChar = "A" Then
                            'Store the device serial number in a global array list to refrence later when the connect button is clicked
                            mSerialList.Add(devList(i).SerialNumber)
                            'Add the serial number and description to the listbox
                            'strip the A off the end of the serial number, and strip the space and A off the end of the description
                            lstDeviceList.Items.Add((devList(i).SerialNumber.Substring(0, devList(i).SerialNumber.Length - 1)) & " - " & devList(i).Description.Substring(0, devList(i).Description.Length - 2))
                        End If
                    End If
                Next
            End If
            If lstDeviceList.Items.Count = 1 Then
                lstDeviceList.SelectedIndex = 0
            End If
        Catch ex As Exception
            MsgBox(ex.Message, MsgBoxStyle.Exclamation, "Error")
        End Try
    End Sub

    Private Sub btnConnect_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnConnect.Click
        Try
            If lstDeviceList.SelectedIndex = -1 Then
                MsgBox("Please select an FTDI device from the list")
                Exit Sub
            End If
            Dim FTStatus As New FTDI.FT_STATUS
            Dim serialNum As String = CType(mSerialList(lstDeviceList.SelectedIndex), String)
            Dim ch1RangeUnits As Integer
            Dim ch1PositionCmdScaled As Double
            Dim ch1PositionCmdCounts As Integer

            FTStatus = mDevice.OpenBySerialNumber(serialNum)
            If FTStatus <> FTDI.FT_STATUS.FT_OK Then
                mConnected = False
                Throw New System.Exception("An exception occured when trying to open a USB connection to S/N " & serialNum & ". FTStatus = " & FTStatus.ToString)
            Else
                mConnected = True
            End If

            'set latency to 2 milliseconds (smallest value available, allows the fastest possible query time for small numbers of memory locations
            FTStatus = mDevice.SetLatency(2)
            If FTStatus <> FTDI.FT_STATUS.FT_OK Then
                mConnected = False
                Throw New System.Exception("An exception occured when trying to set the USB latency parameter")
            End If

            'reset device after setting the latency, otherwise the initial communication will result in an error
            FTStatus = mDevice.ResetDevice
            If FTStatus <> FTDI.FT_STATUS.FT_OK Then
                mConnected = False
                Throw New System.Exception("An exception occured when trying to reset USB device S/N " & serialNum & ". FTStatus = " & FTStatus.ToString)
            End If

            'purge the send and receive buffers
            FTStatus = mDevice.Purge(FTDI.FT_PURGE.FT_PURGE_RX + FTDI.FT_PURGE.FT_PURGE_TX)
            If FTStatus <> FTDI.FT_STATUS.FT_OK Then
                mConnected = False
                Throw New System.Exception("An exception occured when trying to purge send and receive buffers for USB device S/N " & serialNum & ". FTStatus = " & FTStatus.ToString)
            End If

            Dim aStopwatch As New Stopwatch
            aStopwatch.Reset()
            aStopwatch.Start()
            Do While aStopwatch.ElapsedMilliseconds < 60
                'wait for purge to complete
            Loop
            aStopwatch.Stop()

            'determine if the control loop is running, when the control loop is not running the controller is in a startup sequence after being powered on and is not ready yet
            Dim firstCount As Integer
            Dim secondCount As Integer
            Dim thirdCount As Integer
            Do
                firstCount = Me.getInteger(Addresses.loopCount)
                secondCount = Me.getInteger(Addresses.loopCount)
                thirdCount = Me.getInteger(Addresses.loopCount)
            Loop Until firstCount <> secondCount And secondCount <> thirdCount 'query three counts to make sure that the control loop is running rather than reset to zero

            'once the control loop is running, check "startup completed" flag in controller memory ---- value of 0 = not completed, value of 1 = completed
            Dim startupCompleted As Integer
            aStopwatch.Reset()
            aStopwatch.Start()
            Do
                startupCompleted = getInteger(Addresses.gblStartupCompleted)
                If aStopwatch.ElapsedMilliseconds > 10000 Then
                    aStopwatch.Stop()
                    Throw New System.Exception("Timeout waiting for the controller to be ready")
                End If
                Application.DoEvents()
            Loop Until startupCompleted = 1
            aStopwatch.Stop()
            
            If mConnected = True Then
                btnConnect.Visible = False
                btnDisconnect.Visible = True
                btnSendPosition.Visible = True
                btnQuerySensor.Visible = True
                txtPositionCommand.Visible = True
                txtSensorValue.Visible = True
                lblDigitalPosCmd.Visible = True
                lblSensorValue.Visible = True
                lblRangeUnits1.Visible = True
                lblRangeUnits2.Visible = True

                'query Ch1 range for scaling the Digital Position Command and Sensor Reading to and from counts to range units (typically microns)
                mCh1Range = getInteger(Addresses.ch1BaseAddress + Offsets.range)

                'query Ch1 range units type, and set the unit label text accordingly
                ch1RangeUnits = getInteger(Addresses.ch1BaseAddress + Offsets.rangeType)
                Select Case ch1RangeUnits
                    Case 0
                        lblRangeUnits1.Text = "microns"
                        lblRangeUnits2.Text = "microns"
                    Case 1
                        lblRangeUnits1.Text = "millimeters"
                        lblRangeUnits2.Text = "millimeters"
                    Case 2
                        lblRangeUnits1.Text = "µradians"
                        lblRangeUnits2.Text = "µradians"
                    Case 3
                        lblRangeUnits1.Text = "radians"
                        lblRangeUnits2.Text = "radians"
                    Case 4
                        lblRangeUnits1.Text = "nanometers"
                        lblRangeUnits2.Text = "nanometers"
                    Case 5
                        lblRangeUnits1.Text = "milliradians"
                        lblRangeUnits2.Text = "milliradians"
                    Case Else
                        lblRangeUnits1.Text = "microns"
                        lblRangeUnits2.Text = "microns"
                End Select

                'query current Ch1 Digital Position Command and update the text box with the scaled value
                ch1PositionCmdCounts = getInteger(Addresses.ch1BaseAddress + Offsets.digitalPositionCmd)
                ch1PositionCmdScaled = ch1PositionCmdCounts / (1048574 / mCh1Range)
                txtPositionCommand.Text = ch1PositionCmdScaled.ToString("0.000")
                txtSensorValue.Text = ""
            End If
        Catch ex As Exception
            MsgBox(ex.Message, MsgBoxStyle.Exclamation, "Error")
        End Try
    End Sub

    Private Sub btnSendPosition_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnSendPosition.Click
        'scale the digital position command entered in the text box, send to the C400
        Dim ch1DigitalPositionScaled As Single = Single.Parse(txtPositionCommand.Text)
        Dim ch1DigitalPositionCounts As Integer = Convert.ToInt32(Math.Round(ch1DigitalPositionScaled * 1048574 / mCh1Range))
        'make sure that the counts value is not out of range for a 20 bit value
        If ch1DigitalPositionCounts > 524287 Then
            ch1DigitalPositionCounts = 524287
        ElseIf ch1DigitalPositionCounts < -524287 Then
            ch1DigitalPositionCounts = -524287
        End If
        'send the counts value
        setInteger(Addresses.ch1BaseAddress + Offsets.digitalPositionCmd, ch1DigitalPositionCounts)
    End Sub

    Private Sub btnQuerySensor_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnQuerySensor.Click
        'query the sensor reading, populate the text box with a scaled value
        Dim ch1SensorReading As Integer
        Dim scaledSensorReading As Double
        ch1SensorReading = getInteger(Addresses.ch1BaseAddress + Offsets.digitalSensorReading)
        scaledSensorReading = ch1SensorReading / (1048574 / mCh1Range)
        txtSensorValue.Text = scaledSensorReading.ToString("0.000")
    End Sub

    Private Sub btnDisconnect_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnDisconnect.Click
        disconnect()
    End Sub

    Private Sub disconnect()
        Try
            mConnected = False
            btnConnect.Visible = True
            btnDisconnect.Visible = False
            btnSendPosition.Visible = False
            btnQuerySensor.Visible = False
            txtPositionCommand.Visible = False
            txtSensorValue.Visible = False
            lblDigitalPosCmd.Visible = False
            lblSensorValue.Visible = False
            lblRangeUnits1.Visible = False
            lblRangeUnits2.Visible = False
            Dim FTStatus As New FTDI.FT_STATUS
            'disconnect the current open connection
            FTStatus = mDevice.Close
            If FTStatus <> FTDI.FT_STATUS.FT_OK Then
                Throw New System.Exception("An exception occured when closing the USB connection. FTStatus = " & FTStatus.ToString)
            End If
        Catch ex As Exception
            MsgBox(ex.Message, MsgBoxStyle.Exclamation, "Error")
        End Try
    End Sub


    '***************************************************************************************************************************
    'Below are generic functions for reading and writing various data types 1 value at a time.  If speed is critical for reading
    'multiple values, the read/write commands should be stacked in a single buffer and sent at the same time instead
    'of using these functions.
    '***************************************************************************************************************************

    Public Sub setInteger(ByVal memAddress As Integer, ByVal value As Integer)
        Dim msg(9) As Byte
        Dim converterBytes(3) As Byte
        converterBytes = BitConverter.GetBytes(memAddress)
        msg(0) = &HA2
        msg(1) = converterBytes(0)
        msg(2) = converterBytes(1)
        msg(3) = converterBytes(2)
        msg(4) = converterBytes(3)
        converterBytes = BitConverter.GetBytes(value)
        msg(5) = converterBytes(0)
        msg(6) = converterBytes(1)
        msg(7) = converterBytes(2)
        msg(8) = converterBytes(3)
        msg(9) = &H55
        setParameter(msg)
    End Sub

    Public Function getInteger(ByVal memAddress As Integer) As Integer
        Dim msg(5) As Byte
        Dim converterBytes(3) As Byte
        Dim retMsg() As Byte
        converterBytes = BitConverter.GetBytes(memAddress)
        msg(0) = &HA0
        msg(1) = converterBytes(0)
        msg(2) = converterBytes(1)
        msg(3) = converterBytes(2)
        msg(4) = converterBytes(3)
        msg(5) = &H55
        retMsg = queryParameter(msg, 10)
        If retMsg(0) = msg(0) And retMsg(1) = msg(1) And retMsg(9) = &H55 Then
            Return BitConverter.ToInt32(retMsg, 5)
        Else
            Throw New System.Exception("Error - Return message does not follow the expected pattern")
        End If
    End Function

    Public Sub setSingle(ByVal memAddress As Integer, ByVal value As Single)
        Dim msg(9) As Byte
        Dim converterBytes(3) As Byte
        converterBytes = BitConverter.GetBytes(memAddress)
        msg(0) = &HA2
        msg(1) = converterBytes(0)
        msg(2) = converterBytes(1)
        msg(3) = converterBytes(2)
        msg(4) = converterBytes(3)
        converterBytes = BitConverter.GetBytes(value)
        msg(5) = converterBytes(0)
        msg(6) = converterBytes(1)
        msg(7) = converterBytes(2)
        msg(8) = converterBytes(3)
        msg(9) = &H55
        setParameter(msg)
    End Sub

    Public Function getSingle(ByVal memAddress As Integer) As Single
        Dim msg(5) As Byte
        Dim converterBytes(3) As Byte
        Dim retMsg() As Byte
        converterBytes = BitConverter.GetBytes(memAddress)
        msg(0) = &HA0
        msg(1) = converterBytes(0)
        msg(2) = converterBytes(1)
        msg(3) = converterBytes(2)
        msg(4) = converterBytes(3)
        msg(5) = &H55
        retMsg = queryParameter(msg, 10)
        If retMsg(0) = msg(0) And retMsg(1) = msg(1) And retMsg(9) = &H55 Then
            Return BitConverter.ToSingle(retMsg, 5)
        Else
            Throw New System.Exception("Error - Return message does not follow the expected pattern")
        End If
    End Function

    Public Sub setDouble(ByVal memAddress As Integer, ByVal value As Double)
        Dim msg(15) As Byte
        Dim converterBytes(7) As Byte
        Dim addressBytes(3) As Byte
        addressBytes = BitConverter.GetBytes(memAddress)
        msg(0) = &HA2
        msg(1) = addressBytes(0)
        msg(2) = addressBytes(1)
        msg(3) = addressBytes(2)
        msg(4) = addressBytes(3)
        converterBytes = BitConverter.GetBytes(value)
        msg(5) = converterBytes(0)
        msg(6) = converterBytes(1)
        msg(7) = converterBytes(2)
        msg(8) = converterBytes(3)
        msg(9) = &H55
        msg(10) = &HA3
        msg(11) = converterBytes(4)
        msg(12) = converterBytes(5)
        msg(13) = converterBytes(6)
        msg(14) = converterBytes(7)
        msg(15) = &H55
        setParameter(msg)
    End Sub

    Public Function getDouble(ByVal memAddress As Integer) As Double
        Dim msg(9) As Byte
        Dim converterBytes(3) As Byte
        Dim retMsg() As Byte
        converterBytes = BitConverter.GetBytes(memAddress)
        msg(0) = &HA4
        msg(1) = converterBytes(0)
        msg(2) = converterBytes(1)
        msg(3) = converterBytes(2)
        msg(4) = converterBytes(3)
        msg(5) = 2 'read two addresses
        msg(6) = 0
        msg(7) = 0
        msg(8) = 0
        msg(9) = &H55
        retMsg = queryParameter(msg, 14)
        If retMsg(0) = msg(0) And retMsg(1) = msg(1) And retMsg(13) = &H55 Then
            Return BitConverter.ToDouble(retMsg, 5)
        Else
            Throw New System.Exception("Error - Return message does not follow the expected pattern")
        End If
    End Function


    Private Sub setParameter(ByVal msg() As Byte)
        'sends the byte aggray "msg" to the controller without expecting a response from the controller
        Dim FTStatus As New FTDI.FT_STATUS
        Dim bytesWritten As UInteger

        FTStatus = mDevice.Write(msg, msg.Length, bytesWritten)
        If FTStatus <> FTDI.FT_STATUS.FT_OK Then
            Throw New System.Exception("An exception occured when writing to the USB bus. FTStatus = " & FTStatus.ToString)
        End If
    End Sub

    Private Function queryParameter(ByVal msg() As Byte, ByVal numBytesExpected As Integer) As Byte()
        'sends the byte array "msg" to the controller and expects to get a response back.  The response is returned as a byte array.
        'NOTE: not sure yet about the tuning of the variable timeout length

        Dim FTStatus As New FTDI.FT_STATUS
        Dim aStopwatch As New Stopwatch
        Dim rxBuffer(numBytesExpected - 1) As Byte
        Dim partialRX(100000) As Byte
        Dim bytesWritten As UInteger
        Dim bytesAvailable As UInteger
        Dim bytesRead As UInteger
        Dim numReceived As Integer = 0
        Dim timeoutlen As Integer = 200

        'send the msg parameter
        FTStatus = mDevice.Write(msg, msg.Length, bytesWritten)
        If (FTStatus <> FTDI.FT_STATUS.FT_OK) Or (bytesWritten <> msg.Length) Then
            Throw New System.Exception("An exception occured when writing to the USB bus. FTStatus = " & FTStatus.ToString)
        End If

        Do
            aStopwatch.Reset()
            aStopwatch.Start()
            Do
                FTStatus = mDevice.GetRxBytesAvailable(bytesAvailable)
                If FTStatus <> FTDI.FT_STATUS.FT_OK Then
                    Throw New System.Exception("An exception occured when attempting to read an RX byte count. FTStatus = " & FTStatus.ToString)
                    Return Nothing
                End If
                If (aStopwatch.ElapsedMilliseconds > timeoutlen) And ((bytesAvailable + numReceived) < numBytesExpected) Then
                    aStopwatch.Stop()
                    FTStatus = mDevice.Purge(FTDI.FT_PURGE.FT_PURGE_RX) 'purge the RX buffer
                    Throw New System.Exception("A Timeout occured while waiting for the expected number of RX bytes")
                End If
            Loop Until bytesAvailable > 0 'wait until there are bytes to read or we hit the timeout value

            'if available, read bytes from the rx buffer
            If (bytesAvailable + numReceived) > numBytesExpected Then
                aStopwatch.Stop()
                aStopwatch.Reset()
                aStopwatch.Start()
                Do Until aStopwatch.ElapsedMilliseconds >= 200 'wait before purge in case there is more data being sent
                Loop
                aStopwatch.Stop()
                FTStatus = mDevice.Purge(FTDI.FT_PURGE.FT_PURGE_RX) 'purge the RX buffer
                Throw New System.Exception("More bytes than expected in the RX buffer")
            Else
                FTStatus = mDevice.Read(partialRX, bytesAvailable, bytesRead)
                If (FTStatus <> FTDI.FT_STATUS.FT_OK) Then
                    aStopwatch.Stop()
                    Throw New System.Exception("An exception occured when attempting to read from the USB bus. FTStatus = " & FTStatus.ToString)
                Else
                    Array.Copy(partialRX, 0, rxBuffer, numReceived, Convert.ToInt32(bytesRead))
                    numReceived += Convert.ToInt32(bytesRead)
                End If
            End If
        Loop Until numReceived = numBytesExpected 'if we don't have the number of bytes expected yet, loop back and check for more to read
        aStopwatch.Stop()
        Return rxBuffer
    End Function

End Class
