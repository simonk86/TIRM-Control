<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class frmMain
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
        Me.btnListDevices = New System.Windows.Forms.Button()
        Me.lstDeviceList = New System.Windows.Forms.ListBox()
        Me.btnConnect = New System.Windows.Forms.Button()
        Me.btnDisconnect = New System.Windows.Forms.Button()
        Me.txtPositionCommand = New System.Windows.Forms.TextBox()
        Me.btnSendPosition = New System.Windows.Forms.Button()
        Me.lblDigitalPosCmd = New System.Windows.Forms.Label()
        Me.lblSensorValue = New System.Windows.Forms.Label()
        Me.btnQuerySensor = New System.Windows.Forms.Button()
        Me.txtSensorValue = New System.Windows.Forms.TextBox()
        Me.lblRangeUnits1 = New System.Windows.Forms.Label()
        Me.lblRangeUnits2 = New System.Windows.Forms.Label()
        Me.SuspendLayout()
        '
        'btnListDevices
        '
        Me.btnListDevices.Location = New System.Drawing.Point(12, 152)
        Me.btnListDevices.Name = "btnListDevices"
        Me.btnListDevices.Size = New System.Drawing.Size(165, 37)
        Me.btnListDevices.TabIndex = 2
        Me.btnListDevices.Text = "List FTDI Devices"
        Me.btnListDevices.UseVisualStyleBackColor = True
        '
        'lstDeviceList
        '
        Me.lstDeviceList.FormattingEnabled = True
        Me.lstDeviceList.Location = New System.Drawing.Point(12, 12)
        Me.lstDeviceList.Name = "lstDeviceList"
        Me.lstDeviceList.Size = New System.Drawing.Size(165, 134)
        Me.lstDeviceList.TabIndex = 1
        '
        'btnConnect
        '
        Me.btnConnect.Location = New System.Drawing.Point(12, 195)
        Me.btnConnect.Name = "btnConnect"
        Me.btnConnect.Size = New System.Drawing.Size(165, 37)
        Me.btnConnect.TabIndex = 3
        Me.btnConnect.Text = "Connect to Selected Device"
        Me.btnConnect.UseVisualStyleBackColor = True
        '
        'btnDisconnect
        '
        Me.btnDisconnect.Location = New System.Drawing.Point(199, 195)
        Me.btnDisconnect.Name = "btnDisconnect"
        Me.btnDisconnect.Size = New System.Drawing.Size(165, 37)
        Me.btnDisconnect.TabIndex = 8
        Me.btnDisconnect.Text = "Disconnect"
        Me.btnDisconnect.UseVisualStyleBackColor = True
        '
        'txtPositionCommand
        '
        Me.txtPositionCommand.Location = New System.Drawing.Point(355, 35)
        Me.txtPositionCommand.Name = "txtPositionCommand"
        Me.txtPositionCommand.Size = New System.Drawing.Size(87, 20)
        Me.txtPositionCommand.TabIndex = 5
        Me.txtPositionCommand.TextAlign = System.Windows.Forms.HorizontalAlignment.Right
        '
        'btnSendPosition
        '
        Me.btnSendPosition.Location = New System.Drawing.Point(199, 12)
        Me.btnSendPosition.Name = "btnSendPosition"
        Me.btnSendPosition.Size = New System.Drawing.Size(119, 49)
        Me.btnSendPosition.TabIndex = 4
        Me.btnSendPosition.Text = "Send Ch1 Digital Position Command"
        Me.btnSendPosition.UseVisualStyleBackColor = True
        '
        'lblDigitalPosCmd
        '
        Me.lblDigitalPosCmd.AutoSize = True
        Me.lblDigitalPosCmd.Location = New System.Drawing.Point(336, 19)
        Me.lblDigitalPosCmd.Name = "lblDigitalPosCmd"
        Me.lblDigitalPosCmd.Size = New System.Drawing.Size(126, 13)
        Me.lblDigitalPosCmd.TabIndex = 8
        Me.lblDigitalPosCmd.Text = "Digital Position Command"
        '
        'lblSensorValue
        '
        Me.lblSensorValue.AutoSize = True
        Me.lblSensorValue.Location = New System.Drawing.Point(362, 99)
        Me.lblSensorValue.Name = "lblSensorValue"
        Me.lblSensorValue.Size = New System.Drawing.Size(70, 13)
        Me.lblSensorValue.TabIndex = 11
        Me.lblSensorValue.Text = "Sensor Value"
        '
        'btnQuerySensor
        '
        Me.btnQuerySensor.Location = New System.Drawing.Point(199, 92)
        Me.btnQuerySensor.Name = "btnQuerySensor"
        Me.btnQuerySensor.Size = New System.Drawing.Size(119, 49)
        Me.btnQuerySensor.TabIndex = 6
        Me.btnQuerySensor.Text = "Query Ch1 Sensor Value"
        Me.btnQuerySensor.UseVisualStyleBackColor = True
        '
        'txtSensorValue
        '
        Me.txtSensorValue.Location = New System.Drawing.Point(355, 115)
        Me.txtSensorValue.Name = "txtSensorValue"
        Me.txtSensorValue.Size = New System.Drawing.Size(87, 20)
        Me.txtSensorValue.TabIndex = 7
        Me.txtSensorValue.TextAlign = System.Windows.Forms.HorizontalAlignment.Right
        '
        'lblRangeUnits1
        '
        Me.lblRangeUnits1.AutoSize = True
        Me.lblRangeUnits1.Location = New System.Drawing.Point(448, 38)
        Me.lblRangeUnits1.Name = "lblRangeUnits1"
        Me.lblRangeUnits1.Size = New System.Drawing.Size(43, 13)
        Me.lblRangeUnits1.TabIndex = 12
        Me.lblRangeUnits1.Text = "microns"
        '
        'lblRangeUnits2
        '
        Me.lblRangeUnits2.AutoSize = True
        Me.lblRangeUnits2.Location = New System.Drawing.Point(448, 118)
        Me.lblRangeUnits2.Name = "lblRangeUnits2"
        Me.lblRangeUnits2.Size = New System.Drawing.Size(43, 13)
        Me.lblRangeUnits2.TabIndex = 13
        Me.lblRangeUnits2.Text = "microns"
        '
        'frmMain
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(541, 242)
        Me.Controls.Add(Me.lblRangeUnits2)
        Me.Controls.Add(Me.lblRangeUnits1)
        Me.Controls.Add(Me.lblSensorValue)
        Me.Controls.Add(Me.btnQuerySensor)
        Me.Controls.Add(Me.txtSensorValue)
        Me.Controls.Add(Me.lblDigitalPosCmd)
        Me.Controls.Add(Me.btnSendPosition)
        Me.Controls.Add(Me.txtPositionCommand)
        Me.Controls.Add(Me.btnDisconnect)
        Me.Controls.Add(Me.btnConnect)
        Me.Controls.Add(Me.lstDeviceList)
        Me.Controls.Add(Me.btnListDevices)
        Me.Name = "frmMain"
        Me.Text = "C400 Example"
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents btnListDevices As System.Windows.Forms.Button
    Friend WithEvents lstDeviceList As System.Windows.Forms.ListBox
    Friend WithEvents btnConnect As System.Windows.Forms.Button
    Friend WithEvents btnDisconnect As System.Windows.Forms.Button
    Friend WithEvents txtPositionCommand As System.Windows.Forms.TextBox
    Friend WithEvents btnSendPosition As System.Windows.Forms.Button
    Friend WithEvents lblDigitalPosCmd As System.Windows.Forms.Label
    Friend WithEvents lblSensorValue As System.Windows.Forms.Label
    Friend WithEvents btnQuerySensor As System.Windows.Forms.Button
    Friend WithEvents txtSensorValue As System.Windows.Forms.TextBox
    Friend WithEvents lblRangeUnits1 As System.Windows.Forms.Label
    Friend WithEvents lblRangeUnits2 As System.Windows.Forms.Label

End Class
