// Okno.h
#ifndef OKNO_H
#define OKNO_H

#include "NetworkApi.h"
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <thread>
#include <chrono>
#include <mutex>
#include <map>
#include <tlhelp32.h>
#include <psapi.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "psapi.lib")

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;
using namespace System::Net;
using namespace System::Net::Sockets;
using namespace System::Threading;
using namespace System::IO;
using namespace System::Diagnostics;
using namespace System::Runtime::InteropServices;
using namespace System::Globalization;
using namespace System::Net::NetworkInformation;
using namespace System::Xml;
using namespace System::Collections::Generic;

namespace Monitoringseti {

    public ref struct ProcessTrafficInfo
    {
        String^ ProcessName;
        int ProcessId;
        long long BytesSent;
        long long BytesReceived;
        long long LastBytesSent;
        long long LastBytesReceived;
        DateTime LastUpdate;
        long long LastSpeedSent;
        long long LastSpeedReceived;

        ProcessTrafficInfo()
        {
            ProcessName = "";
            ProcessId = 0;
            BytesSent = 0;
            BytesReceived = 0;
            LastBytesSent = 0;
            LastBytesReceived = 0;
            LastUpdate = DateTime::Now;
            LastSpeedSent = 0;
            LastSpeedReceived = 0;
        }

        long long GetCurrentSpeedSent()
        {
            return LastSpeedSent;
        }

        long long GetCurrentSpeedReceived()
        {
            return LastSpeedReceived;
        }

        void CalculateSpeed()
        {
            TimeSpan diff = DateTime::Now - LastUpdate;
            if (diff.TotalSeconds < 0.1)
            {
                LastSpeedSent = 0;
                LastSpeedReceived = 0;
                return;
            }

            LastSpeedSent = (long long)((BytesSent - LastBytesSent) / diff.TotalSeconds);
            LastSpeedReceived = (long long)((BytesReceived - LastBytesReceived) / diff.TotalSeconds);

            if (LastSpeedSent < 0) LastSpeedSent = 0;
            if (LastSpeedReceived < 0) LastSpeedReceived = 0;
        }
    };

    private ref class ProcessTrafficInfoComparer
    {
    public:
        static int Compare(ProcessTrafficInfo^ a, ProcessTrafficInfo^ b)
        {
            long long totalA = a->GetCurrentSpeedSent() + a->GetCurrentSpeedReceived();
            long long totalB = b->GetCurrentSpeedSent() + b->GetCurrentSpeedReceived();
            return totalB.CompareTo(totalA);
        }
    };

    private ref class ScrollState
    {
    public:
        int FirstDisplayedRowIndex;
        int HorizontalOffset;
        int SelectedRowIndex;

        ScrollState()
        {
            FirstDisplayedRowIndex = -1;
            HorizontalOffset = 0;
            SelectedRowIndex = -1;
        }
    };

    public ref class NetworkNode
    {
    public:
        String^ IPAddress;
        String^ HostName;
        long long ResponseTime;
        int PacketLoss;
        bool IsAvailable;
        DateTime LastCheck;

        NetworkNode()
        {
            IPAddress = "";
            HostName = "";
            ResponseTime = 0;
            PacketLoss = 0;
            IsAvailable = false;
            LastCheck = DateTime::Now;
        }
    };

    public ref class ScanParameters
    {
    public:
        String^ Host;
        int StartPort;
        int EndPort;
        bool ScanTCP;
        bool ScanUDP;
        int Timeout;
    };

    public ref class ConnectionData
    {
    public:
        String^ Protocol;
        String^ LocalAddr;
        int LocalPort;
        String^ RemoteAddr;
        int RemotePort;
        String^ State;
        int ProcessId;
        String^ ProcessName;

        ConnectionData(String^ proto, String^ localAddr, int localPort,
            String^ remoteAddr, int remotePort, String^ state, int pid, String^ procName)
        {
            Protocol = proto;
            LocalAddr = localAddr;
            LocalPort = localPort;
            RemoteAddr = remoteAddr;
            RemotePort = remotePort;
            State = state;
            ProcessId = pid;
            ProcessName = procName;
        }
    };

    public ref class Okno : public System::Windows::Forms::Form
    {
    private:
        System::Windows::Forms::TabControl^ tabControl1;
        System::Windows::Forms::TabPage^ tabDashboard;
        System::Windows::Forms::TabPage^ tabTraffic;
        System::Windows::Forms::TabPage^ tabConnections;
        System::Windows::Forms::TabPage^ tabNodes;
        System::Windows::Forms::TabPage^ tabPortScan;
        System::Windows::Forms::TabPage^ tabLog;
        System::Windows::Forms::TabPage^ tabInfo;

        System::Windows::Forms::Label^ lblNetworkStatus;
        System::Windows::Forms::Label^ lblPublicIP;
        System::Windows::Forms::Label^ lblUptime;
        System::Windows::Forms::Label^ lblCurrentTime;
        System::Windows::Forms::Label^ lblActiveConnectionsCount;
        System::Windows::Forms::Label^ lblMonitoredNodesCount;
        System::Windows::Forms::Timer^ timerClock;

        System::Windows::Forms::Label^ lblDownloadSpeed;
        System::Windows::Forms::Label^ lblUploadSpeed;
        System::Windows::Forms::Label^ lblTotalDownload;
        System::Windows::Forms::Label^ lblTotalUpload;
        System::Windows::Forms::ProgressBar^ progressBarDownload;
        System::Windows::Forms::ProgressBar^ progressBarUpload;
        System::Windows::Forms::Timer^ timerTraffic;

        System::Windows::Forms::DataGridView^ dgvConnections;
        System::Windows::Forms::Button^ btnRefreshConnectionsList;
        System::Windows::Forms::Timer^ timerConnections;
        System::Windows::Forms::TextBox^ txtConnectionFilter;
        System::Windows::Forms::Button^ btnFilterConnections;
        System::Windows::Forms::ComboBox^ cmbProtocolFilter;

        System::Windows::Forms::DataGridView^ dgvNodes;
        System::Windows::Forms::Button^ btnAddMonitoringNode;
        System::Windows::Forms::Button^ btnRemoveMonitoringNode;
        System::Windows::Forms::Button^ btnManualPing;
        System::Windows::Forms::Button^ btnScanSelectedNode;
        System::Windows::Forms::TextBox^ txtNewNodeIP;
        System::Windows::Forms::Timer^ timerNodes;
        System::Windows::Forms::CheckBox^ chkBackgroundMonitoring;
        System::Windows::Forms::Button^ btnImportNodes;
        System::Windows::Forms::Button^ btnExportNodes;

        System::Windows::Forms::TextBox^ txtScanHost;
        System::Windows::Forms::NumericUpDown^ numStartPort;
        System::Windows::Forms::NumericUpDown^ numEndPort;
        System::Windows::Forms::CheckBox^ chkScanTCP;
        System::Windows::Forms::CheckBox^ chkScanUDP;
        System::Windows::Forms::Button^ btnStartPortScan;
        System::Windows::Forms::Button^ btnSelectFromNodes;
        System::Windows::Forms::ListBox^ lbOpenPorts;
        System::Windows::Forms::ProgressBar^ progressScan;
        System::Windows::Forms::Label^ lblScanStatus;
        System::Windows::Forms::NumericUpDown^ numTimeout;

        System::Windows::Forms::RichTextBox^ rtbLog;
        System::Windows::Forms::CheckBox^ chkSoundAlerts;
        System::Windows::Forms::CheckBox^ chkPopupAlerts;
        System::Windows::Forms::CheckBox^ chkLogTraffic;
        System::Windows::Forms::Button^ btnClearEventLog;
        System::Windows::Forms::Button^ btnSaveLog;
        System::Windows::Forms::Button^ btnChooseLogPath;
        System::Windows::Forms::Label^ lblLogPath;
        System::Windows::Forms::ComboBox^ cmbLogLevel;

        System::Windows::Forms::PictureBox^ pbDownloadChart;
        System::Windows::Forms::PictureBox^ pbUploadChart;
        System::Windows::Forms::Label^ lblChartLegend;

        long long totalDownloadBytes;
        long long totalUploadBytes;
        DateTime lastTrafficLogTime;
        DateTime startTime;

        System::Collections::Generic::List<NetworkNode^>^ nodes;
        System::Collections::Generic::List<ConnectionData^>^ allConnections;
        System::Collections::Generic::List<ConnectionData^>^ filteredConnections;

        int lastSelectedNodeIndex;
        int selectedNodeIndex;
        int selectedConnectionIndex;

        StreamWriter^ logWriter;
        Object^ logLock;
        bool isScanning;
        String^ configFilePath;
        String^ currentLogPath;
        String^ publicIP;

        System::Windows::Forms::ListBox^ lbAvailableNodes;
        System::Windows::Forms::ListBox^ lbActiveConns;
        System::Windows::Forms::Label^ lblTotalTraffic;

        System::Windows::Forms::Label^ lblDashboardUptime;
        System::Windows::Forms::Label^ lblDashboardPublicIP;
        System::Windows::Forms::Label^ lblDashboardCurrentTime;
        System::Windows::Forms::Label^ lblDashboardNodesCount;
        System::Windows::Forms::Label^ lblDashboardConnectionsCount;

        int savedActiveConnsTopIndex;
        int savedAvailableNodesTopIndex;

        int trafficCounter;
        int connectionsCounter;
        Random^ random;

        const int CHART_POINTS_COUNT = 50;
        System::Collections::Generic::Queue<int>^ downloadHistory;
        System::Collections::Generic::Queue<int>^ uploadHistory;

        Dictionary<int, ProcessTrafficInfo^>^ processTrafficData;
        Dictionary<int, String^>^ processNameCache;
        Dictionary<int, String^>^ processPathCache;
        Object^ trafficDataLock;

        ScrollState^ connectionsScrollState;

        const int SMOOTHING_BUFFER_SIZE = 5;
        System::Collections::Generic::Queue<long long>^ downloadSpeedBuffer;
        System::Collections::Generic::Queue<long long>^ uploadSpeedBuffer;

        Dictionary<int, long long>^ lastBytesSent;
        Dictionary<int, long long>^ lastBytesReceived;
        Dictionary<int, DateTime>^ lastUpdateTime;

        void EnableDoubleBuffering(DataGridView^ dgv)
        {
            if (dgv != nullptr)
            {
                System::Reflection::PropertyInfo^ dgvProp = dgv->GetType()->GetProperty("DoubleBuffered",
                    System::Reflection::BindingFlags::Instance | System::Reflection::BindingFlags::NonPublic);
                if (dgvProp != nullptr)
                {
                    dgvProp->SetValue(dgv, true, nullptr);
                }
            }
        }

        void EnableListBoxDoubleBuffering(ListBox^ listBox)
        {
            if (listBox != nullptr)
            {
                System::Reflection::PropertyInfo^ property = listBox->GetType()->GetProperty("DoubleBuffered",
                    System::Reflection::BindingFlags::Instance | System::Reflection::BindingFlags::NonPublic);
                if (property != nullptr)
                {
                    property->SetValue(listBox, true, nullptr);
                }
            }
        }

        long long GetSmoothedSpeed(System::Collections::Generic::Queue<long long>^ buffer, long long newValue)
        {
            buffer->Enqueue(newValue);
            if (buffer->Count > SMOOTHING_BUFFER_SIZE)
                buffer->Dequeue();

            long long sum = 0;
            for each (long long val in buffer)
                sum += val;

            return sum / buffer->Count;
        }

        void GetNetworkSpeed(long long% downloadSpeedKB, long long% uploadSpeedKB)
        {
            downloadSpeedKB = 0;
            uploadSpeedKB = 0;

            static long long lastDownloadBytes = 0;
            static long long lastUploadBytes = 0;
            static LARGE_INTEGER liFrequency = { 0 };
            static LARGE_INTEGER liLastTime = { 0 };
            static bool isFirstRun = true;

            try
            {
                long long currentDownloadBytes = 0;
                long long currentUploadBytes = 0;

                array<NetworkInterface^>^ interfaces = NetworkInterface::GetAllNetworkInterfaces();

                for each (NetworkInterface ^ netIf in interfaces)
                {
                    if (netIf->OperationalStatus == OperationalStatus::Up &&
                        netIf->NetworkInterfaceType != NetworkInterfaceType::Loopback)
                    {
                        IPv4InterfaceStatistics^ stats = netIf->GetIPv4Statistics();
                        if (stats != nullptr)
                        {
                            currentDownloadBytes += stats->BytesReceived;
                            currentUploadBytes += stats->BytesSent;
                        }
                    }
                }

                if (liFrequency.QuadPart == 0)
                {
                    QueryPerformanceFrequency(&liFrequency);
                    QueryPerformanceCounter(&liLastTime);
                    isFirstRun = true;
                }
                else
                {
                    LARGE_INTEGER liCurrentTime;
                    QueryPerformanceCounter(&liCurrentTime);

                    double seconds = (double)(liCurrentTime.QuadPart - liLastTime.QuadPart) / liFrequency.QuadPart;
                    liLastTime = liCurrentTime;

                    if (!isFirstRun && seconds > 0.5 && lastDownloadBytes > 0)
                    {
                        long long downDiff = currentDownloadBytes - lastDownloadBytes;
                        long long upDiff = currentUploadBytes - lastUploadBytes;

                        if (downDiff < 0) downDiff = 0;
                        if (upDiff < 0) upDiff = 0;

                        downloadSpeedKB = (long long)(downDiff / seconds / 1024);
                        uploadSpeedKB = (long long)(upDiff / seconds / 1024);

                        if (downloadSpeedKB < 0) downloadSpeedKB = 0;
                        if (uploadSpeedKB < 0) uploadSpeedKB = 0;

                        totalDownloadBytes += downDiff;
                        totalUploadBytes += upDiff;
                    }

                    isFirstRun = false;
                }

                lastDownloadBytes = currentDownloadBytes;
                lastUploadBytes = currentUploadBytes;
            }
            catch (Exception^) {}
        }

    public:
        Okno(void)
        {
            InitializeComponent();

            this->DoubleBuffered = true;
            this->SetStyle(ControlStyles::AllPaintingInWmPaint | ControlStyles::UserPaint | ControlStyles::DoubleBuffer, true);
            this->UpdateStyles();

            nodes = gcnew System::Collections::Generic::List<NetworkNode^>();
            allConnections = gcnew System::Collections::Generic::List<ConnectionData^>();
            filteredConnections = gcnew System::Collections::Generic::List<ConnectionData^>();

            totalDownloadBytes = 0;
            totalUploadBytes = 0;
            logLock = gcnew Object();
            isScanning = false;
            lastSelectedNodeIndex = -1;
            selectedNodeIndex = -1;
            selectedConnectionIndex = -1;
            lastTrafficLogTime = DateTime::Now;
            startTime = DateTime::Now;
            publicIP = "Не определен";

            savedActiveConnsTopIndex = -1;
            savedAvailableNodesTopIndex = -1;
            trafficCounter = 0;
            connectionsCounter = 0;
            random = gcnew Random();

            configFilePath = Application::StartupPath + "\\nodes_config.xml";
            currentLogPath = Application::StartupPath + "\\network_monitor.log";

            connectionsScrollState = gcnew ScrollState();

            processTrafficData = gcnew Dictionary<int, ProcessTrafficInfo^>();
            processNameCache = gcnew Dictionary<int, String^>();
            processPathCache = gcnew Dictionary<int, String^>();
            trafficDataLock = gcnew Object();

            lastBytesSent = gcnew Dictionary<int, long long>();
            lastBytesReceived = gcnew Dictionary<int, long long>();
            lastUpdateTime = gcnew Dictionary<int, DateTime>();

            downloadSpeedBuffer = gcnew System::Collections::Generic::Queue<long long>();
            uploadSpeedBuffer = gcnew System::Collections::Generic::Queue<long long>();

            downloadHistory = gcnew System::Collections::Generic::Queue<int>();
            uploadHistory = gcnew System::Collections::Generic::Queue<int>();
            for (int i = 0; i < CHART_POINTS_COUNT; i++) {
                downloadHistory->Enqueue(0);
                uploadHistory->Enqueue(0);
            }

            try
            {
                logWriter = gcnew StreamWriter(currentLogPath, true);
                logWriter->AutoFlush = true;
                LogMessage("Программа запущена");
                if (lblLogPath != nullptr)
                    lblLogPath->Text = "Путь к логу: " + currentLogPath;
            }
            catch (Exception^) {}

            LoadNodesFromFile();
            if (nodes->Count == 0)
            {
                LoadDefaultNodes();
                SaveNodesToFile();
            }

            EnableListBoxDoubleBuffering(lbAvailableNodes);
            EnableListBoxDoubleBuffering(lbActiveConns);

            UpdateRealConnections();
            ApplyFilter();
            StartTrafficMonitoring();
            GetPublicIP();
            UpdateNetworkStatus();
            LogMessage(String::Format("Загружено узлов для мониторинга: {0}", nodes->Count));

            UpdateNodesGrid();
            UpdateConnectionsGrid();

            if (timerClock != nullptr)
            {
                timerClock->Start();
            }

            this->Shown += gcnew EventHandler(this, &Okno::Okno_Shown);
        }

    protected:
        ~Okno()
        {
            SaveNodesToFile();
            LogTrafficSummary();
            LogMessage("Программа завершена");
            if (logWriter) delete logWriter;
            if (timerTraffic) timerTraffic->Stop();
            if (timerConnections) timerConnections->Stop();
            if (timerNodes) timerNodes->Stop();
            if (timerClock) timerClock->Stop();

            if (components) delete components;
        }

    private:
        System::ComponentModel::Container^ components;

        void InitializeComponent(void)
        {
            this->components = gcnew System::ComponentModel::Container();
            this->tabControl1 = gcnew System::Windows::Forms::TabControl();
            this->tabDashboard = gcnew System::Windows::Forms::TabPage();
            this->tabTraffic = gcnew System::Windows::Forms::TabPage();
            this->tabConnections = gcnew System::Windows::Forms::TabPage();
            this->tabNodes = gcnew System::Windows::Forms::TabPage();
            this->tabPortScan = gcnew System::Windows::Forms::TabPage();
            this->tabLog = gcnew System::Windows::Forms::TabPage();
            this->tabInfo = gcnew System::Windows::Forms::TabPage();

            this->SuspendLayout();

            this->tabControl1->Dock = DockStyle::Fill;
            this->tabControl1->TabPages->Add(tabDashboard);
            this->tabControl1->TabPages->Add(tabTraffic);
            this->tabControl1->TabPages->Add(tabConnections);
            this->tabControl1->TabPages->Add(tabNodes);
            this->tabControl1->TabPages->Add(tabPortScan);
            this->tabControl1->TabPages->Add(tabLog);
            this->tabControl1->TabPages->Add(tabInfo);

            this->tabControl1->ItemSize = System::Drawing::Size(0, 50);

            this->tabDashboard->Text = L"Главная";
            this->tabTraffic->Text = L"Трафик";
            this->tabConnections->Text = L"Соединения";
            this->tabNodes->Text = L"Узлы";
            this->tabPortScan->Text = L"Сканер портов";
            this->tabLog->Text = L"Журнал";
            this->tabInfo->Text = L"Информация";

            InitializeDashboardTab();
            InitializeTrafficTab();
            InitializeConnectionsTab();
            InitializeNodesTab();
            InitializePortScanTab();
            InitializeLogTab();
            InitializeInfoTab();

            this->ClientSize = System::Drawing::Size(1366, 850);
            this->MinimumSize = System::Drawing::Size(1024, 768);
            this->Text = L"Мониторинг и анализ сети v2.0";
            this->BackColor = System::Drawing::Color::FromArgb(240, 242, 245);
            this->StartPosition = FormStartPosition::CenterScreen;
            this->Controls->Add(this->tabControl1);
            this->ResumeLayout(false);

            timerClock = gcnew System::Windows::Forms::Timer();
            timerClock->Interval = 1000;
            timerClock->Tick += gcnew EventHandler(this, &Okno::timerClock_Tick);
        }

        void InitializeDashboardTab()
        {
            this->tabDashboard->BackColor = System::Drawing::Color::FromArgb(240, 242, 245);
            this->tabDashboard->Controls->Clear();

            System::Windows::Forms::Label^ lblWelcome = gcnew System::Windows::Forms::Label();
            lblWelcome->Text = L"СЕТЕВОЙ МОНИТОР ПРОФЕССИОНАЛ";
            lblWelcome->Location = System::Drawing::Point(30, 30);
            lblWelcome->Size = System::Drawing::Size(700, 40);
            lblWelcome->Font = gcnew System::Drawing::Font("Segoe UI", 20, System::Drawing::FontStyle::Bold);
            lblWelcome->ForeColor = System::Drawing::Color::FromArgb(0, 120, 215);

            System::Windows::Forms::Label^ lblSubtitle = gcnew System::Windows::Forms::Label();
            lblSubtitle->Text = L"Комплексное решение для мониторинга и анализа сети";
            lblSubtitle->Location = System::Drawing::Point(30, 75);
            lblSubtitle->Size = System::Drawing::Size(550, 25);
            lblSubtitle->Font = gcnew System::Drawing::Font("Segoe UI", 11);
            lblSubtitle->ForeColor = System::Drawing::Color::Gray;

            System::Windows::Forms::GroupBox^ gbStatus = gcnew System::Windows::Forms::GroupBox();
            gbStatus->Text = L"Текущий статус сети";
            gbStatus->Location = System::Drawing::Point(30, 115);
            gbStatus->Size = System::Drawing::Size(450, 200);
            gbStatus->Font = gcnew System::Drawing::Font("Segoe UI", 11, System::Drawing::FontStyle::Bold);
            gbStatus->BackColor = System::Drawing::Color::White;

            System::Windows::Forms::Label^ lblStatusIcon = gcnew System::Windows::Forms::Label();
            lblStatusIcon->Text = L"";
            lblStatusIcon->Location = System::Drawing::Point(20, 35);
            lblStatusIcon->Size = System::Drawing::Size(40, 40);
            lblStatusIcon->Font = gcnew System::Drawing::Font("Segoe UI", 28);

            System::Windows::Forms::Label^ lblStatusText = gcnew System::Windows::Forms::Label();
            lblStatusText->Text = L"Статус: ПОДКЛЮЧЕН";
            lblStatusText->Location = System::Drawing::Point(70, 45);
            lblStatusText->Size = System::Drawing::Size(300, 30);
            lblStatusText->Font = gcnew System::Drawing::Font("Segoe UI", 12, System::Drawing::FontStyle::Bold);
            lblStatusText->ForeColor = System::Drawing::Color::FromArgb(40, 167, 69);

            System::Windows::Forms::Label^ lblUptimeIcon = gcnew System::Windows::Forms::Label();
            lblUptimeIcon->Text = L"";
            lblUptimeIcon->Location = System::Drawing::Point(20, 85);
            lblUptimeIcon->Size = System::Drawing::Size(30, 30);
            lblUptimeIcon->Font = gcnew System::Drawing::Font("Segoe UI", 18);

            lblDashboardUptime = gcnew System::Windows::Forms::Label();
            lblDashboardUptime->Text = L"Время работы: 0 ч 0 мин 0 сек";
            lblDashboardUptime->Location = System::Drawing::Point(60, 90);
            lblDashboardUptime->Size = System::Drawing::Size(350, 25);
            lblDashboardUptime->Font = gcnew System::Drawing::Font("Segoe UI", 10);

            System::Windows::Forms::Label^ lblPublicIPIcon = gcnew System::Windows::Forms::Label();
            lblPublicIPIcon->Text = L"";
            lblPublicIPIcon->Location = System::Drawing::Point(20, 120);
            lblPublicIPIcon->Size = System::Drawing::Size(30, 30);
            lblPublicIPIcon->Font = gcnew System::Drawing::Font("Segoe UI", 18);

            lblDashboardPublicIP = gcnew System::Windows::Forms::Label();
            lblDashboardPublicIP->Text = L"Внешний IP: Загрузка...";
            lblDashboardPublicIP->Location = System::Drawing::Point(60, 125);
            lblDashboardPublicIP->Size = System::Drawing::Size(350, 25);
            lblDashboardPublicIP->Font = gcnew System::Drawing::Font("Segoe UI", 10);

            System::Windows::Forms::Label^ lblTimeIcon = gcnew System::Windows::Forms::Label();
            lblTimeIcon->Text = L"";
            lblTimeIcon->Location = System::Drawing::Point(20, 155);
            lblTimeIcon->Size = System::Drawing::Size(30, 30);
            lblTimeIcon->Font = gcnew System::Drawing::Font("Segoe UI", 18);

            lblDashboardCurrentTime = gcnew System::Windows::Forms::Label();
            lblDashboardCurrentTime->Text = DateTime::Now.ToString("dd.MM.yyyy HH:mm:ss");
            lblDashboardCurrentTime->Location = System::Drawing::Point(60, 160);
            lblDashboardCurrentTime->Size = System::Drawing::Size(350, 25);
            lblDashboardCurrentTime->Font = gcnew System::Drawing::Font("Segoe UI", 10);

            gbStatus->Controls->Add(lblStatusIcon);
            gbStatus->Controls->Add(lblStatusText);
            gbStatus->Controls->Add(lblUptimeIcon);
            gbStatus->Controls->Add(lblDashboardUptime);
            gbStatus->Controls->Add(lblPublicIPIcon);
            gbStatus->Controls->Add(lblDashboardPublicIP);
            gbStatus->Controls->Add(lblTimeIcon);
            gbStatus->Controls->Add(lblDashboardCurrentTime);

            System::Windows::Forms::GroupBox^ gbStats = gcnew System::Windows::Forms::GroupBox();
            gbStats->Text = L"Быстрая статистика";
            gbStats->Location = System::Drawing::Point(510, 115);
            gbStats->Size = System::Drawing::Size(250, 200);
            gbStats->Font = gcnew System::Drawing::Font("Segoe UI", 11, System::Drawing::FontStyle::Bold);
            gbStats->BackColor = System::Drawing::Color::White;

            System::Windows::Forms::Label^ lblNodesIcon = gcnew System::Windows::Forms::Label();
            lblNodesIcon->Text = L"";
            lblNodesIcon->Location = System::Drawing::Point(20, 35);
            lblNodesIcon->Size = System::Drawing::Size(40, 35);
            lblNodesIcon->Font = gcnew System::Drawing::Font("Segoe UI", 22);

            System::Windows::Forms::Label^ lblNodesLabel = gcnew System::Windows::Forms::Label();
            lblNodesLabel->Text = L"Наблюдаемых узлов";
            lblNodesLabel->Location = System::Drawing::Point(70, 35);
            lblNodesLabel->Size = System::Drawing::Size(170, 20);
            lblNodesLabel->Font = gcnew System::Drawing::Font("Segoe UI", 9);
            lblNodesLabel->ForeColor = System::Drawing::Color::Gray;

            lblDashboardNodesCount = gcnew System::Windows::Forms::Label();
            lblDashboardNodesCount->Text = L"0";
            lblDashboardNodesCount->Location = System::Drawing::Point(70, 55);
            lblDashboardNodesCount->Size = System::Drawing::Size(100, 30);
            lblDashboardNodesCount->Font = gcnew System::Drawing::Font("Segoe UI", 18, System::Drawing::FontStyle::Bold);
            lblDashboardNodesCount->ForeColor = System::Drawing::Color::FromArgb(0, 120, 215);

            System::Windows::Forms::Label^ lblConnectionsIcon = gcnew System::Windows::Forms::Label();
            lblConnectionsIcon->Text = L"";
            lblConnectionsIcon->Location = System::Drawing::Point(20, 110);
            lblConnectionsIcon->Size = System::Drawing::Size(40, 35);
            lblConnectionsIcon->Font = gcnew System::Drawing::Font("Segoe UI", 22);

            System::Windows::Forms::Label^ lblConnectionsLabel = gcnew System::Windows::Forms::Label();
            lblConnectionsLabel->Text = L"Активных соединений";
            lblConnectionsLabel->Location = System::Drawing::Point(70, 110);
            lblConnectionsLabel->Size = System::Drawing::Size(170, 20);
            lblConnectionsLabel->Font = gcnew System::Drawing::Font("Segoe UI", 9);
            lblConnectionsLabel->ForeColor = System::Drawing::Color::Gray;

            lblDashboardConnectionsCount = gcnew System::Windows::Forms::Label();
            lblDashboardConnectionsCount->Text = L"0";
            lblDashboardConnectionsCount->Location = System::Drawing::Point(70, 130);
            lblDashboardConnectionsCount->Size = System::Drawing::Size(100, 30);
            lblDashboardConnectionsCount->Font = gcnew System::Drawing::Font("Segoe UI", 18, System::Drawing::FontStyle::Bold);
            lblDashboardConnectionsCount->ForeColor = System::Drawing::Color::FromArgb(0, 120, 215);

            gbStats->Controls->Add(lblNodesIcon);
            gbStats->Controls->Add(lblNodesLabel);
            gbStats->Controls->Add(lblDashboardNodesCount);
            gbStats->Controls->Add(lblConnectionsIcon);
            gbStats->Controls->Add(lblConnectionsLabel);
            gbStats->Controls->Add(lblDashboardConnectionsCount);

            System::Windows::Forms::GroupBox^ gbActions = gcnew System::Windows::Forms::GroupBox();
            gbActions->Text = L"Быстрые действия";
            gbActions->Location = System::Drawing::Point(790, 115);
            gbActions->Size = System::Drawing::Size(250, 205);
            gbActions->Font = gcnew System::Drawing::Font("Segoe UI", 11, System::Drawing::FontStyle::Bold);
            gbActions->BackColor = System::Drawing::Color::White;

            System::Windows::Forms::Button^ btnQuickScan = gcnew System::Windows::Forms::Button();
            btnQuickScan->Text = L"Сканировать порты";
            btnQuickScan->Location = System::Drawing::Point(15, 30);
            btnQuickScan->Size = System::Drawing::Size(220, 35);
            btnQuickScan->BackColor = System::Drawing::Color::FromArgb(0, 123, 255);
            btnQuickScan->ForeColor = System::Drawing::Color::White;
            btnQuickScan->FlatStyle = FlatStyle::Flat;
            btnQuickScan->Font = gcnew System::Drawing::Font("Segoe UI", 9);
            btnQuickScan->Click += gcnew EventHandler(this, &Okno::btnQuickScan_Click);

            System::Windows::Forms::Button^ btnQuickPing = gcnew System::Windows::Forms::Button();
            btnQuickPing->Text = L"Проверить все узлы";
            btnQuickPing->Location = System::Drawing::Point(15, 75);
            btnQuickPing->Size = System::Drawing::Size(220, 35);
            btnQuickPing->BackColor = System::Drawing::Color::FromArgb(40, 167, 69);
            btnQuickPing->ForeColor = System::Drawing::Color::White;
            btnQuickPing->FlatStyle = FlatStyle::Flat;
            btnQuickPing->Font = gcnew System::Drawing::Font("Segoe UI", 9);
            btnQuickPing->Click += gcnew EventHandler(this, &Okno::btnQuickPing_Click);

            System::Windows::Forms::Button^ btnQuickExport = gcnew System::Windows::Forms::Button();
            btnQuickExport->Text = L"Экспорт отчёта";
            btnQuickExport->Location = System::Drawing::Point(15, 120);
            btnQuickExport->Size = System::Drawing::Size(220, 35);
            btnQuickExport->BackColor = System::Drawing::Color::FromArgb(108, 117, 125);
            btnQuickExport->ForeColor = System::Drawing::Color::White;
            btnQuickExport->FlatStyle = FlatStyle::Flat;
            btnQuickExport->Font = gcnew System::Drawing::Font("Segoe UI", 9);
            btnQuickExport->Click += gcnew EventHandler(this, &Okno::btnQuickExport_Click);

            System::Windows::Forms::Button^ btnRefreshDashboard = gcnew System::Windows::Forms::Button();
            btnRefreshDashboard->Text = L"Обновить графики";
            btnRefreshDashboard->Location = System::Drawing::Point(15, 165);
            btnRefreshDashboard->Size = System::Drawing::Size(220, 35);
            btnRefreshDashboard->BackColor = System::Drawing::Color::FromArgb(23, 162, 184);
            btnRefreshDashboard->ForeColor = System::Drawing::Color::White;
            btnRefreshDashboard->FlatStyle = FlatStyle::Flat;
            btnRefreshDashboard->Font = gcnew System::Drawing::Font("Segoe UI", 9);
            btnRefreshDashboard->Click += gcnew EventHandler(this, &Okno::btnRefreshDashboard_Click);

            gbActions->Controls->Add(btnQuickScan);
            gbActions->Controls->Add(btnQuickPing);
            gbActions->Controls->Add(btnQuickExport);
            gbActions->Controls->Add(btnRefreshDashboard);

            System::Windows::Forms::GroupBox^ gbCharts = gcnew System::Windows::Forms::GroupBox();
            gbCharts->Text = L"График загрузки сети (последние 50 сек)";
            gbCharts->Location = System::Drawing::Point(30, 330);
            gbCharts->Size = System::Drawing::Size(1010, 250);
            gbCharts->Font = gcnew System::Drawing::Font("Segoe UI", 11, System::Drawing::FontStyle::Bold);
            gbCharts->BackColor = System::Drawing::Color::White;

            System::Windows::Forms::Label^ lblDownloadChartTitle = gcnew System::Windows::Forms::Label();
            lblDownloadChartTitle->Text = L"Загрузка (KB/s)";
            lblDownloadChartTitle->Location = System::Drawing::Point(20, 25);
            lblDownloadChartTitle->Size = System::Drawing::Size(200, 25);
            lblDownloadChartTitle->Font = gcnew System::Drawing::Font("Segoe UI", 10, System::Drawing::FontStyle::Bold);
            lblDownloadChartTitle->ForeColor = System::Drawing::Color::FromArgb(40, 167, 69);

            pbDownloadChart = gcnew System::Windows::Forms::PictureBox();
            pbDownloadChart->Location = System::Drawing::Point(20, 55);
            pbDownloadChart->Size = System::Drawing::Size(480, 150);
            pbDownloadChart->BackColor = System::Drawing::Color::FromArgb(245, 245, 245);
            pbDownloadChart->BorderStyle = BorderStyle::FixedSingle;

            System::Windows::Forms::Label^ lblUploadChartTitle = gcnew System::Windows::Forms::Label();
            lblUploadChartTitle->Text = L"Отдача (KB/s)";
            lblUploadChartTitle->Location = System::Drawing::Point(520, 25);
            lblUploadChartTitle->Size = System::Drawing::Size(200, 25);
            lblUploadChartTitle->Font = gcnew System::Drawing::Font("Segoe UI", 10, System::Drawing::FontStyle::Bold);
            lblUploadChartTitle->ForeColor = System::Drawing::Color::FromArgb(220, 53, 69);

            pbUploadChart = gcnew System::Windows::Forms::PictureBox();
            pbUploadChart->Location = System::Drawing::Point(520, 55);
            pbUploadChart->Size = System::Drawing::Size(470, 150);
            pbUploadChart->BackColor = System::Drawing::Color::FromArgb(245, 245, 245);
            pbUploadChart->BorderStyle = BorderStyle::FixedSingle;

            lblChartLegend = gcnew System::Windows::Forms::Label();
            lblChartLegend->Text = L"Зелёная линия - скорость загрузки, Красная линия - скорость отдачи. Шкала: 0 - 10 MB/s";
            lblChartLegend->Location = System::Drawing::Point(20, 215);
            lblChartLegend->Size = System::Drawing::Size(970, 25);
            lblChartLegend->Font = gcnew System::Drawing::Font("Segoe UI", 8);
            lblChartLegend->ForeColor = System::Drawing::Color::Gray;

            gbCharts->Controls->Add(lblDownloadChartTitle);
            gbCharts->Controls->Add(pbDownloadChart);
            gbCharts->Controls->Add(lblUploadChartTitle);
            gbCharts->Controls->Add(pbUploadChart);
            gbCharts->Controls->Add(lblChartLegend);

            System::Windows::Forms::GroupBox^ gbAvailableNodes = gcnew System::Windows::Forms::GroupBox();
            gbAvailableNodes->Text = L"Доступные узлы";
            gbAvailableNodes->Location = System::Drawing::Point(30, 595);
            gbAvailableNodes->Size = System::Drawing::Size(300, 200);
            gbAvailableNodes->Font = gcnew System::Drawing::Font("Segoe UI", 11, System::Drawing::FontStyle::Bold);
            gbAvailableNodes->BackColor = System::Drawing::Color::White;

            lbAvailableNodes = gcnew System::Windows::Forms::ListBox();
            lbAvailableNodes->Location = System::Drawing::Point(15, 35);
            lbAvailableNodes->Size = System::Drawing::Size(250, 140);
            lbAvailableNodes->Font = gcnew System::Drawing::Font("Consolas", 10);
            lbAvailableNodes->BackColor = System::Drawing::Color::FromArgb(250, 250, 250);
            lbAvailableNodes->IntegralHeight = false;

            gbAvailableNodes->Controls->Add(lbAvailableNodes);

            System::Windows::Forms::GroupBox^ gbActiveConns = gcnew System::Windows::Forms::GroupBox();
            gbActiveConns->Text = L"Активные соединения";
            gbActiveConns->Location = System::Drawing::Point(350, 595);
            gbActiveConns->Size = System::Drawing::Size(700, 200);
            gbActiveConns->Font = gcnew System::Drawing::Font("Segoe UI", 11, System::Drawing::FontStyle::Bold);
            gbActiveConns->BackColor = System::Drawing::Color::White;

            lbActiveConns = gcnew System::Windows::Forms::ListBox();
            lbActiveConns->Location = System::Drawing::Point(15, 35);
            lbActiveConns->Size = System::Drawing::Size(650, 140);
            lbActiveConns->Font = gcnew System::Drawing::Font("Consolas", 9);
            lbActiveConns->BackColor = System::Drawing::Color::FromArgb(250, 250, 250);
            lbActiveConns->IntegralHeight = false;

            gbActiveConns->Controls->Add(lbActiveConns);

            this->tabDashboard->Controls->Add(lblWelcome);
            this->tabDashboard->Controls->Add(lblSubtitle);
            this->tabDashboard->Controls->Add(gbStatus);
            this->tabDashboard->Controls->Add(gbStats);
            this->tabDashboard->Controls->Add(gbActions);
            this->tabDashboard->Controls->Add(gbCharts);
            this->tabDashboard->Controls->Add(gbAvailableNodes);
            this->tabDashboard->Controls->Add(gbActiveConns);
        }

        void InitializeTrafficTab()
        {
            this->tabTraffic->BackColor = System::Drawing::Color::FromArgb(240, 242, 245);
            this->tabTraffic->Controls->Clear();
            this->tabTraffic->AutoScroll = true;

            System::Windows::Forms::Label^ lblTrafficTitle = gcnew System::Windows::Forms::Label();
            lblTrafficTitle->Text = L"МОНИТОРИНГ СЕТЕВОГО ТРАФИКА";
            lblTrafficTitle->Location = System::Drawing::Point(30, 30);
            lblTrafficTitle->Size = System::Drawing::Size(550, 35);
            lblTrafficTitle->Font = gcnew System::Drawing::Font("Segoe UI", 14, System::Drawing::FontStyle::Bold);
            lblTrafficTitle->ForeColor = System::Drawing::Color::FromArgb(0, 120, 215);

            System::Windows::Forms::Label^ lblSubtitle = gcnew System::Windows::Forms::Label();
            lblSubtitle->Text = L"Реальное время: скорость загрузки и отдачи обновляются каждую секунду";
            lblSubtitle->Location = System::Drawing::Point(30, 65);
            lblSubtitle->Size = System::Drawing::Size(600, 25);
            lblSubtitle->Font = gcnew System::Drawing::Font("Segoe UI", 9);
            lblSubtitle->ForeColor = System::Drawing::Color::Gray;

            System::Windows::Forms::GroupBox^ gbSpeed = gcnew System::Windows::Forms::GroupBox();
            gbSpeed->Text = L"Текущая скорость";
            gbSpeed->Location = System::Drawing::Point(30, 105);
            gbSpeed->Size = System::Drawing::Size(500, 230);
            gbSpeed->Font = gcnew System::Drawing::Font("Segoe UI", 11, System::Drawing::FontStyle::Bold);
            gbSpeed->BackColor = System::Drawing::Color::White;

            System::Windows::Forms::Label^ lblDownloadIcon = gcnew System::Windows::Forms::Label();
            lblDownloadIcon->Text = L"";
            lblDownloadIcon->Location = System::Drawing::Point(20, 30);
            lblDownloadIcon->Size = System::Drawing::Size(50, 45);
            lblDownloadIcon->Font = gcnew System::Drawing::Font("Segoe UI", 32);

            System::Windows::Forms::Label^ lblDownloadLabel = gcnew System::Windows::Forms::Label();
            lblDownloadLabel->Text = L"Загрузка";
            lblDownloadLabel->Location = System::Drawing::Point(80, 30);
            lblDownloadLabel->Size = System::Drawing::Size(100, 25);
            lblDownloadLabel->Font = gcnew System::Drawing::Font("Segoe UI", 10);
            lblDownloadLabel->ForeColor = System::Drawing::Color::Gray;

            lblDownloadSpeed = gcnew System::Windows::Forms::Label();
            lblDownloadSpeed->Text = L"0 KB/s (0.00 MB/s)";
            lblDownloadSpeed->Location = System::Drawing::Point(80, 55);
            lblDownloadSpeed->Size = System::Drawing::Size(380, 45);
            lblDownloadSpeed->Font = gcnew System::Drawing::Font("Segoe UI", 18, System::Drawing::FontStyle::Bold);
            lblDownloadSpeed->ForeColor = System::Drawing::Color::FromArgb(40, 167, 69);
            lblDownloadSpeed->AutoEllipsis = true;

            progressBarDownload = gcnew System::Windows::Forms::ProgressBar();
            progressBarDownload->Location = System::Drawing::Point(20, 110);
            progressBarDownload->Size = System::Drawing::Size(460, 25);
            progressBarDownload->Maximum = 10240;

            System::Windows::Forms::Label^ lblUploadIcon = gcnew System::Windows::Forms::Label();
            lblUploadIcon->Text = L"";
            lblUploadIcon->Location = System::Drawing::Point(20, 150);
            lblUploadIcon->Size = System::Drawing::Size(50, 45);
            lblUploadIcon->Font = gcnew System::Drawing::Font("Segoe UI", 32);

            System::Windows::Forms::Label^ lblUploadLabel = gcnew System::Windows::Forms::Label();
            lblUploadLabel->Text = L"Отдача";
            lblUploadLabel->Location = System::Drawing::Point(80, 150);
            lblUploadLabel->Size = System::Drawing::Size(100, 25);
            lblUploadLabel->Font = gcnew System::Drawing::Font("Segoe UI", 10);
            lblUploadLabel->ForeColor = System::Drawing::Color::Gray;

            lblUploadSpeed = gcnew System::Windows::Forms::Label();
            lblUploadSpeed->Text = L"0 KB/s (0.00 MB/s)";
            lblUploadSpeed->Location = System::Drawing::Point(80, 175);
            lblUploadSpeed->Size = System::Drawing::Size(380, 45);
            lblUploadSpeed->Font = gcnew System::Drawing::Font("Segoe UI", 18, System::Drawing::FontStyle::Bold);
            lblUploadSpeed->ForeColor = System::Drawing::Color::FromArgb(220, 53, 69);
            lblUploadSpeed->AutoEllipsis = true;

            progressBarUpload = gcnew System::Windows::Forms::ProgressBar();
            progressBarUpload->Location = System::Drawing::Point(20, 230);
            progressBarUpload->Size = System::Drawing::Size(460, 25);
            progressBarUpload->Maximum = 10240;

            gbSpeed->Controls->Add(lblDownloadIcon);
            gbSpeed->Controls->Add(lblDownloadLabel);
            gbSpeed->Controls->Add(lblDownloadSpeed);
            gbSpeed->Controls->Add(progressBarDownload);
            gbSpeed->Controls->Add(lblUploadIcon);
            gbSpeed->Controls->Add(lblUploadLabel);
            gbSpeed->Controls->Add(lblUploadSpeed);
            gbSpeed->Controls->Add(progressBarUpload);

            System::Windows::Forms::GroupBox^ gbTotal = gcnew System::Windows::Forms::GroupBox();
            gbTotal->Text = L"Общий трафик за сессию";
            gbTotal->Location = System::Drawing::Point(560, 105);
            gbTotal->Size = System::Drawing::Size(280, 230);
            gbTotal->Font = gcnew System::Drawing::Font("Segoe UI", 11, System::Drawing::FontStyle::Bold);
            gbTotal->BackColor = System::Drawing::Color::White;

            System::Windows::Forms::Label^ lblTotalDownloadIcon = gcnew System::Windows::Forms::Label();
            lblTotalDownloadIcon->Text = L"";
            lblTotalDownloadIcon->Location = System::Drawing::Point(20, 30);
            lblTotalDownloadIcon->Size = System::Drawing::Size(50, 35);
            lblTotalDownloadIcon->Font = gcnew System::Drawing::Font("Segoe UI", 24);

            System::Windows::Forms::Label^ lblTotalDownloadLabel = gcnew System::Windows::Forms::Label();
            lblTotalDownloadLabel->Text = L"Всего загружено";
            lblTotalDownloadLabel->Location = System::Drawing::Point(80, 35);
            lblTotalDownloadLabel->Size = System::Drawing::Size(150, 25);
            lblTotalDownloadLabel->Font = gcnew System::Drawing::Font("Segoe UI", 9);
            lblTotalDownloadLabel->ForeColor = System::Drawing::Color::Gray;

            lblTotalDownload = gcnew System::Windows::Forms::Label();
            lblTotalDownload->Text = L"0 MB";
            lblTotalDownload->Location = System::Drawing::Point(80, 60);
            lblTotalDownload->Size = System::Drawing::Size(180, 40);
            lblTotalDownload->Font = gcnew System::Drawing::Font("Segoe UI", 16, System::Drawing::FontStyle::Bold);
            lblTotalDownload->ForeColor = System::Drawing::Color::FromArgb(40, 167, 69);

            System::Windows::Forms::Label^ lblTotalUploadIcon = gcnew System::Windows::Forms::Label();
            lblTotalUploadIcon->Text = L"";
            lblTotalUploadIcon->Location = System::Drawing::Point(20, 115);
            lblTotalUploadIcon->Size = System::Drawing::Size(50, 35);
            lblTotalUploadIcon->Font = gcnew System::Drawing::Font("Segoe UI", 24);

            System::Windows::Forms::Label^ lblTotalUploadLabel = gcnew System::Windows::Forms::Label();
            lblTotalUploadLabel->Text = L"Всего отправлено";
            lblTotalUploadLabel->Location = System::Drawing::Point(80, 120);
            lblTotalUploadLabel->Size = System::Drawing::Size(150, 25);
            lblTotalUploadLabel->Font = gcnew System::Drawing::Font("Segoe UI", 9);
            lblTotalUploadLabel->ForeColor = System::Drawing::Color::Gray;

            lblTotalUpload = gcnew System::Windows::Forms::Label();
            lblTotalUpload->Text = L"0 MB";
            lblTotalUpload->Location = System::Drawing::Point(80, 145);
            lblTotalUpload->Size = System::Drawing::Size(180, 40);
            lblTotalUpload->Font = gcnew System::Drawing::Font("Segoe UI", 16, System::Drawing::FontStyle::Bold);
            lblTotalUpload->ForeColor = System::Drawing::Color::FromArgb(220, 53, 69);

            gbTotal->Controls->Add(lblTotalDownloadIcon);
            gbTotal->Controls->Add(lblTotalDownloadLabel);
            gbTotal->Controls->Add(lblTotalDownload);
            gbTotal->Controls->Add(lblTotalUploadIcon);
            gbTotal->Controls->Add(lblTotalUploadLabel);
            gbTotal->Controls->Add(lblTotalUpload);

            System::Windows::Forms::GroupBox^ gbTrafficControls = gcnew System::Windows::Forms::GroupBox();
            gbTrafficControls->Text = L"Управление";
            gbTrafficControls->Location = System::Drawing::Point(30, 350);
            gbTrafficControls->Size = System::Drawing::Size(810, 80);
            gbTrafficControls->Font = gcnew System::Drawing::Font("Segoe UI", 11, System::Drawing::FontStyle::Bold);
            gbTrafficControls->BackColor = System::Drawing::Color::White;

            System::Windows::Forms::Button^ btnResetTrafficStats = gcnew System::Windows::Forms::Button();
            btnResetTrafficStats->Text = L"Сбросить статистику трафика";
            btnResetTrafficStats->Location = System::Drawing::Point(20, 25);
            btnResetTrafficStats->Size = System::Drawing::Size(200, 40);
            btnResetTrafficStats->BackColor = System::Drawing::Color::FromArgb(255, 193, 7);
            btnResetTrafficStats->ForeColor = System::Drawing::Color::Black;
            btnResetTrafficStats->FlatStyle = FlatStyle::Flat;
            btnResetTrafficStats->Font = gcnew System::Drawing::Font("Segoe UI", 9);
            btnResetTrafficStats->Click += gcnew EventHandler(this, &Okno::btnResetTrafficStats_Click);

            System::Windows::Forms::Button^ btnExportTrafficReport = gcnew System::Windows::Forms::Button();
            btnExportTrafficReport->Text = L"Экспорт отчёта по трафику";
            btnExportTrafficReport->Location = System::Drawing::Point(240, 25);
            btnExportTrafficReport->Size = System::Drawing::Size(200, 40);
            btnExportTrafficReport->BackColor = System::Drawing::Color::FromArgb(40, 167, 69);
            btnExportTrafficReport->ForeColor = System::Drawing::Color::White;
            btnExportTrafficReport->FlatStyle = FlatStyle::Flat;
            btnExportTrafficReport->Font = gcnew System::Drawing::Font("Segoe UI", 9);
            btnExportTrafficReport->Click += gcnew EventHandler(this, &Okno::btnExportTrafficReport_Click);

            System::Windows::Forms::Button^ btnShowTrafficInfo = gcnew System::Windows::Forms::Button();
            btnShowTrafficInfo->Text = L"Что означают цифры?";
            btnShowTrafficInfo->Location = System::Drawing::Point(460, 25);
            btnShowTrafficInfo->Size = System::Drawing::Size(200, 40);
            btnShowTrafficInfo->BackColor = System::Drawing::Color::FromArgb(108, 117, 125);
            btnShowTrafficInfo->ForeColor = System::Drawing::Color::White;
            btnShowTrafficInfo->FlatStyle = FlatStyle::Flat;
            btnShowTrafficInfo->Font = gcnew System::Drawing::Font("Segoe UI", 9);
            btnShowTrafficInfo->Click += gcnew EventHandler(this, &Okno::btnShowTrafficInfo_Click);

            gbTrafficControls->Controls->Add(btnResetTrafficStats);
            gbTrafficControls->Controls->Add(btnExportTrafficReport);
            gbTrafficControls->Controls->Add(btnShowTrafficInfo);

            System::Windows::Forms::GroupBox^ gbTrafficTip = gcnew System::Windows::Forms::GroupBox();
            gbTrafficTip->Text = L"Как найти приложение, которое активно использует сеть";
            gbTrafficTip->Location = System::Drawing::Point(35, 435);
            gbTrafficTip->Size = System::Drawing::Size(810, 100);
            gbTrafficTip->Font = gcnew System::Drawing::Font("Segoe UI", 11, System::Drawing::FontStyle::Bold);
            gbTrafficTip->BackColor = System::Drawing::Color::FromArgb(255, 248, 225);

            System::Windows::Forms::Label^ lblTrafficTipIcon = gcnew System::Windows::Forms::Label();
            lblTrafficTipIcon->Text = L"";
            lblTrafficTipIcon->Location = System::Drawing::Point(15, 25);
            lblTrafficTipIcon->Size = System::Drawing::Size(50, 45);
            lblTrafficTipIcon->Font = gcnew System::Drawing::Font("Segoe UI", 32);

            System::Windows::Forms::Label^ lblTrafficTipText = gcnew System::Windows::Forms::Label();
            lblTrafficTipText->Text = L"1. Перейдите на вкладку 'Соединения' - там показаны ВСЕ процессы, использующие сеть.\n"
                L"2. Обратите внимание на процессы с большим количеством соединений (ESTABLISHED).\n"
                L"3. Сортируйте таблицу по столбцу 'Процесс' или 'Удаленный адрес' для удобства поиска.\n"
                L"4. Если подозреваете вирус - ищите неизвестные процессы (например, без цифровой подписи).";
            lblTrafficTipText->Location = System::Drawing::Point(75, 20);
            lblTrafficTipText->Size = System::Drawing::Size(720, 70);
            lblTrafficTipText->Font = gcnew System::Drawing::Font("Segoe UI", 9);
            lblTrafficTipText->ForeColor = System::Drawing::Color::FromArgb(102, 77, 30);

            gbTrafficTip->Controls->Add(lblTrafficTipIcon);
            gbTrafficTip->Controls->Add(lblTrafficTipText);

            this->tabTraffic->Controls->Add(lblTrafficTitle);
            this->tabTraffic->Controls->Add(lblSubtitle);
            this->tabTraffic->Controls->Add(gbSpeed);
            this->tabTraffic->Controls->Add(gbTotal);
            this->tabTraffic->Controls->Add(gbTrafficControls);
            this->tabTraffic->Controls->Add(gbTrafficTip);
        }

        void InitializeConnectionsTab()
        {
            this->tabConnections->BackColor = System::Drawing::Color::FromArgb(240, 242, 245);

            System::Windows::Forms::Panel^ panelFilters = gcnew System::Windows::Forms::Panel();
            panelFilters->Location = System::Drawing::Point(10, 35);
            panelFilters->Size = System::Drawing::Size(960, 40);
            panelFilters->BackColor = System::Drawing::Color::White;

            System::Windows::Forms::Label^ lblFilter = gcnew System::Windows::Forms::Label();
            lblFilter->Text = L"Фильтр:";
            lblFilter->Location = System::Drawing::Point(10, 10);
            lblFilter->Size = System::Drawing::Size(70, 25);
            lblFilter->Font = gcnew System::Drawing::Font("Segoe UI", 9);

            txtConnectionFilter = gcnew System::Windows::Forms::TextBox();
            txtConnectionFilter->Location = System::Drawing::Point(80, 8);
            txtConnectionFilter->Size = System::Drawing::Size(180, 25);
            txtConnectionFilter->Text = L"Введите IP, порт или процесс...";
            txtConnectionFilter->ForeColor = System::Drawing::Color::Gray;
            txtConnectionFilter->Enter += gcnew EventHandler(this, &Okno::txtFilter_Enter);
            txtConnectionFilter->Leave += gcnew EventHandler(this, &Okno::txtFilter_Leave);

            System::Windows::Forms::Label^ lblProtocol = gcnew System::Windows::Forms::Label();
            lblProtocol->Text = L"Протокол:";
            lblProtocol->Location = System::Drawing::Point(280, 10);
            lblProtocol->Size = System::Drawing::Size(80, 25);
            lblProtocol->Font = gcnew System::Drawing::Font("Segoe UI", 9);

            cmbProtocolFilter = gcnew System::Windows::Forms::ComboBox();
            cmbProtocolFilter->Location = System::Drawing::Point(360, 8);
            cmbProtocolFilter->Size = System::Drawing::Size(100, 25);
            cmbProtocolFilter->Items->Add(L"Все");
            cmbProtocolFilter->Items->Add(L"TCP");
            cmbProtocolFilter->Items->Add(L"UDP");
            cmbProtocolFilter->SelectedIndex = 0;

            btnFilterConnections = gcnew System::Windows::Forms::Button();
            btnFilterConnections->Text = L"Применить";
            btnFilterConnections->Location = System::Drawing::Point(480, 7);
            btnFilterConnections->Size = System::Drawing::Size(110, 27);
            btnFilterConnections->BackColor = System::Drawing::Color::FromArgb(0, 120, 215);
            btnFilterConnections->ForeColor = System::Drawing::Color::White;
            btnFilterConnections->FlatStyle = FlatStyle::Flat;
            btnFilterConnections->Click += gcnew EventHandler(this, &Okno::btnFilterConnections_Click);

            btnRefreshConnectionsList = gcnew System::Windows::Forms::Button();
            btnRefreshConnectionsList->Text = L"Обновить";
            btnRefreshConnectionsList->Location = System::Drawing::Point(600, 7);
            btnRefreshConnectionsList->Size = System::Drawing::Size(110, 27);
            btnRefreshConnectionsList->BackColor = System::Drawing::Color::FromArgb(0, 120, 215);
            btnRefreshConnectionsList->ForeColor = System::Drawing::Color::White;
            btnRefreshConnectionsList->FlatStyle = FlatStyle::Flat;
            btnRefreshConnectionsList->Click += gcnew EventHandler(this, &Okno::btnRefreshConnections_Click);

            panelFilters->Controls->Add(lblFilter);
            panelFilters->Controls->Add(txtConnectionFilter);
            panelFilters->Controls->Add(lblProtocol);
            panelFilters->Controls->Add(cmbProtocolFilter);
            panelFilters->Controls->Add(btnFilterConnections);
            panelFilters->Controls->Add(btnRefreshConnectionsList);

            dgvConnections = gcnew System::Windows::Forms::DataGridView();
            dgvConnections->Location = System::Drawing::Point(10, 85);
            dgvConnections->Size = System::Drawing::Size(960, 520);
            dgvConnections->AllowUserToAddRows = false;
            dgvConnections->ReadOnly = true;
            dgvConnections->BackgroundColor = System::Drawing::Color::White;
            dgvConnections->BorderStyle = BorderStyle::FixedSingle;
            dgvConnections->GridColor = System::Drawing::Color::FromArgb(230, 230, 230);
            dgvConnections->SelectionMode = DataGridViewSelectionMode::FullRowSelect;
            dgvConnections->MultiSelect = false;
            dgvConnections->VirtualMode = false;

            EnableDoubleBuffering(dgvConnections);

            System::Windows::Forms::Panel^ panelLegend = gcnew System::Windows::Forms::Panel();
            panelLegend->Location = System::Drawing::Point(10, 615);
            panelLegend->Size = System::Drawing::Size(960, 80);
            panelLegend->BackColor = System::Drawing::Color::FromArgb(255, 255, 225);
            panelLegend->BorderStyle = BorderStyle::FixedSingle;

            System::Windows::Forms::Label^ lblLegendTitle = gcnew System::Windows::Forms::Label();
            lblLegendTitle->Text = L"Обозначение цветов строк в таблице:";
            lblLegendTitle->Location = System::Drawing::Point(10, 8);
            lblLegendTitle->Size = System::Drawing::Size(400, 20);
            lblLegendTitle->Font = gcnew System::Drawing::Font("Segoe UI", 9, System::Drawing::FontStyle::Bold);
            lblLegendTitle->ForeColor = System::Drawing::Color::FromArgb(0, 70, 120);

            System::Windows::Forms::Label^ lblGreenBox = gcnew System::Windows::Forms::Label();
            lblGreenBox->Text = L"  ";
            lblGreenBox->Location = System::Drawing::Point(3, 35);
            lblGreenBox->Size = System::Drawing::Size(20, 20);
            lblGreenBox->BackColor = System::Drawing::Color::FromArgb(200, 255, 200);
            lblGreenBox->BorderStyle = BorderStyle::FixedSingle;

            System::Windows::Forms::Label^ lblGreenText = gcnew System::Windows::Forms::Label();
            lblGreenText->Text = L"Зелёный: АКТИВНОЕ соединение (ESTABLISHED).";
            lblGreenText->Location = System::Drawing::Point(25, 37);
            lblGreenText->Size = System::Drawing::Size(330, 20);
            lblGreenText->Font = gcnew System::Drawing::Font("Segoe UI", 8);

            System::Windows::Forms::Label^ lblYellowBox = gcnew System::Windows::Forms::Label();
            lblYellowBox->Text = L"  ";
            lblYellowBox->Location = System::Drawing::Point(360, 35);
            lblYellowBox->Size = System::Drawing::Size(20, 20);
            lblYellowBox->BackColor = System::Drawing::Color::FromArgb(255, 230, 150);
            lblYellowBox->BorderStyle = BorderStyle::FixedSingle;

            System::Windows::Forms::Label^ lblYellowText = gcnew System::Windows::Forms::Label();
            lblYellowText->Text = L"Жёлтый: процесс имеет 10-19 соединений.";
            lblYellowText->Location = System::Drawing::Point(385, 37);
            lblYellowText->Size = System::Drawing::Size(300, 20);
            lblYellowText->Font = gcnew System::Drawing::Font("Segoe UI", 8);

            System::Windows::Forms::Label^ lblRedBox = gcnew System::Windows::Forms::Label();
            lblRedBox->Text = L"  ";
            lblRedBox->Location = System::Drawing::Point(690, 35);
            lblRedBox->Size = System::Drawing::Size(20, 20);
            lblRedBox->BackColor = System::Drawing::Color::FromArgb(255, 200, 200);
            lblRedBox->BorderStyle = BorderStyle::FixedSingle;

            System::Windows::Forms::Label^ lblRedText = gcnew System::Windows::Forms::Label();
            lblRedText->Text = L"Красный: процесс имеет 20+ соединений.";
            lblRedText->Location = System::Drawing::Point(715, 37);
            lblRedText->Size = System::Drawing::Size(300, 20);
            lblRedText->Font = gcnew System::Drawing::Font("Segoe UI", 8);

            System::Windows::Forms::Label^ lblWhiteBox = gcnew System::Windows::Forms::Label();
            lblWhiteBox->Text = L"  ";
            lblWhiteBox->Location = System::Drawing::Point(3, 58);
            lblWhiteBox->Size = System::Drawing::Size(20, 20);
            lblWhiteBox->BackColor = System::Drawing::Color::White;
            lblWhiteBox->BorderStyle = BorderStyle::FixedSingle;

            System::Windows::Forms::Label^ lblWhiteText = gcnew System::Windows::Forms::Label();
            lblWhiteText->Text = L"Белый: ПАССИВНОЕ соединение (LISTENING/ожидание).";
            lblWhiteText->Location = System::Drawing::Point(25, 60);
            lblWhiteText->Size = System::Drawing::Size(400, 20);
            lblWhiteText->Font = gcnew System::Drawing::Font("Segoe UI", 8);

            panelLegend->Controls->Add(lblLegendTitle);
            panelLegend->Controls->Add(lblGreenBox);
            panelLegend->Controls->Add(lblGreenText);
            panelLegend->Controls->Add(lblYellowBox);
            panelLegend->Controls->Add(lblYellowText);
            panelLegend->Controls->Add(lblRedBox);
            panelLegend->Controls->Add(lblRedText);
            panelLegend->Controls->Add(lblWhiteBox);
            panelLegend->Controls->Add(lblWhiteText);

            this->tabConnections->Controls->Add(panelFilters);
            this->tabConnections->Controls->Add(dgvConnections);
            this->tabConnections->Controls->Add(panelLegend);
        }

        void InitializeNodesTab()
        {
            this->tabNodes->BackColor = System::Drawing::Color::FromArgb(240, 242, 245);

            dgvNodes = gcnew System::Windows::Forms::DataGridView();
            dgvNodes->Location = System::Drawing::Point(10, 70);
            dgvNodes->Size = System::Drawing::Size(960, 480);
            dgvNodes->AllowUserToAddRows = false;
            dgvNodes->ReadOnly = true;
            dgvNodes->BackgroundColor = System::Drawing::Color::White;
            dgvNodes->BorderStyle = BorderStyle::FixedSingle;
            dgvNodes->SelectionMode = DataGridViewSelectionMode::FullRowSelect;
            dgvNodes->MultiSelect = false;
            dgvNodes->VirtualMode = false;

            EnableDoubleBuffering(dgvNodes);

            dgvNodes->RowEnter += gcnew System::Windows::Forms::DataGridViewCellEventHandler(this, &Okno::dgvNodes_RowEnter);

            System::Windows::Forms::Label^ lblNodesTitle = gcnew System::Windows::Forms::Label();
            lblNodesTitle->Text = L"Управление наблюдаемыми узлами";
            lblNodesTitle->Location = System::Drawing::Point(10, 30);
            lblNodesTitle->Size = System::Drawing::Size(350, 25);
            lblNodesTitle->Font = gcnew System::Drawing::Font("Segoe UI", 10, System::Drawing::FontStyle::Bold);

            txtNewNodeIP = gcnew System::Windows::Forms::TextBox();
            txtNewNodeIP->Location = System::Drawing::Point(10, 580);
            txtNewNodeIP->Size = System::Drawing::Size(200, 25);
            txtNewNodeIP->Text = L"Введите IP или домен...";
            txtNewNodeIP->ForeColor = System::Drawing::Color::Gray;
            txtNewNodeIP->Enter += gcnew EventHandler(this, &Okno::txtNewNodeIP_Enter);
            txtNewNodeIP->Leave += gcnew EventHandler(this, &Okno::txtNewNodeIP_Leave);

            btnAddMonitoringNode = gcnew System::Windows::Forms::Button();
            btnAddMonitoringNode->Text = L"Добавить";
            btnAddMonitoringNode->Location = System::Drawing::Point(220, 579);
            btnAddMonitoringNode->Size = System::Drawing::Size(120, 27);
            btnAddMonitoringNode->BackColor = System::Drawing::Color::FromArgb(40, 167, 69);
            btnAddMonitoringNode->ForeColor = System::Drawing::Color::White;
            btnAddMonitoringNode->FlatStyle = FlatStyle::Flat;
            btnAddMonitoringNode->Click += gcnew EventHandler(this, &Okno::btnAddNode_Click);

            btnRemoveMonitoringNode = gcnew System::Windows::Forms::Button();
            btnRemoveMonitoringNode->Text = L"Удалить";
            btnRemoveMonitoringNode->Location = System::Drawing::Point(350, 579);
            btnRemoveMonitoringNode->Size = System::Drawing::Size(120, 27);
            btnRemoveMonitoringNode->BackColor = System::Drawing::Color::FromArgb(220, 53, 69);
            btnRemoveMonitoringNode->ForeColor = System::Drawing::Color::White;
            btnRemoveMonitoringNode->FlatStyle = FlatStyle::Flat;
            btnRemoveMonitoringNode->Click += gcnew EventHandler(this, &Okno::btnRemoveNode_Click);

            btnManualPing = gcnew System::Windows::Forms::Button();
            btnManualPing->Text = L"Пинг";
            btnManualPing->Location = System::Drawing::Point(480, 579);
            btnManualPing->Size = System::Drawing::Size(120, 27);
            btnManualPing->BackColor = System::Drawing::Color::FromArgb(255, 193, 7);
            btnManualPing->ForeColor = System::Drawing::Color::Black;
            btnManualPing->FlatStyle = FlatStyle::Flat;
            btnManualPing->Click += gcnew EventHandler(this, &Okno::btnPingSelected_Click);

            btnScanSelectedNode = gcnew System::Windows::Forms::Button();
            btnScanSelectedNode->Text = L"Сканировать порты";
            btnScanSelectedNode->Location = System::Drawing::Point(610, 579);
            btnScanSelectedNode->Size = System::Drawing::Size(140, 27);
            btnScanSelectedNode->BackColor = System::Drawing::Color::FromArgb(0, 123, 255);
            btnScanSelectedNode->ForeColor = System::Drawing::Color::White;
            btnScanSelectedNode->FlatStyle = FlatStyle::Flat;
            btnScanSelectedNode->Click += gcnew EventHandler(this, &Okno::btnScanSelectedNode_Click);

            btnImportNodes = gcnew System::Windows::Forms::Button();
            btnImportNodes->Text = L"Импорт";
            btnImportNodes->Location = System::Drawing::Point(760, 579);
            btnImportNodes->Size = System::Drawing::Size(90, 27);
            btnImportNodes->BackColor = System::Drawing::Color::FromArgb(108, 117, 125);
            btnImportNodes->ForeColor = System::Drawing::Color::White;
            btnImportNodes->FlatStyle = FlatStyle::Flat;
            btnImportNodes->Click += gcnew EventHandler(this, &Okno::btnImportNodes_Click);

            btnExportNodes = gcnew System::Windows::Forms::Button();
            btnExportNodes->Text = L"Экспорт";
            btnExportNodes->Location = System::Drawing::Point(860, 579);
            btnExportNodes->Size = System::Drawing::Size(90, 27);
            btnExportNodes->BackColor = System::Drawing::Color::FromArgb(108, 117, 125);
            btnExportNodes->ForeColor = System::Drawing::Color::White;
            btnExportNodes->FlatStyle = FlatStyle::Flat;
            btnExportNodes->Click += gcnew EventHandler(this, &Okno::btnExportNodes_Click);

            chkBackgroundMonitoring = gcnew System::Windows::Forms::CheckBox();
            chkBackgroundMonitoring->Text = L"Фоновый мониторинг (автообновление статуса)";
            chkBackgroundMonitoring->Location = System::Drawing::Point(10, 620);
            chkBackgroundMonitoring->Size = System::Drawing::Size(400, 25);
            chkBackgroundMonitoring->Checked = true;
            chkBackgroundMonitoring->CheckStateChanged += gcnew EventHandler(this, &Okno::chkBackgroundMonitoring_Changed);

            lblMonitoredNodesCount = gcnew System::Windows::Forms::Label();
            lblMonitoredNodesCount->Text = L"Наблюдаемых узлов: 0";
            lblMonitoredNodesCount->Location = System::Drawing::Point(10, 660);
            lblMonitoredNodesCount->Size = System::Drawing::Size(350, 25);
            lblMonitoredNodesCount->Font = gcnew System::Drawing::Font("Segoe UI", 10);

            lblActiveConnectionsCount = gcnew System::Windows::Forms::Label();
            lblActiveConnectionsCount->Text = L"Активных соединений: 0";
            lblActiveConnectionsCount->Location = System::Drawing::Point(320, 660);
            lblActiveConnectionsCount->Size = System::Drawing::Size(350, 25);
            lblActiveConnectionsCount->Font = gcnew System::Drawing::Font("Segoe UI", 10);

            timerNodes = gcnew System::Windows::Forms::Timer();
            timerNodes->Interval = 5000;
            timerNodes->Tick += gcnew EventHandler(this, &Okno::timerNodes_Tick);

            this->tabNodes->Controls->Add(lblNodesTitle);
            this->tabNodes->Controls->Add(dgvNodes);
            this->tabNodes->Controls->Add(txtNewNodeIP);
            this->tabNodes->Controls->Add(btnAddMonitoringNode);
            this->tabNodes->Controls->Add(btnRemoveMonitoringNode);
            this->tabNodes->Controls->Add(btnManualPing);
            this->tabNodes->Controls->Add(btnScanSelectedNode);
            this->tabNodes->Controls->Add(btnImportNodes);
            this->tabNodes->Controls->Add(btnExportNodes);
            this->tabNodes->Controls->Add(chkBackgroundMonitoring);
            this->tabNodes->Controls->Add(lblMonitoredNodesCount);
            this->tabNodes->Controls->Add(lblActiveConnectionsCount);
        }

        void InitializePortScanTab()
        {
            this->tabPortScan->BackColor = System::Drawing::Color::FromArgb(240, 242, 245);
            this->tabPortScan->AutoScroll = true;

            System::Windows::Forms::GroupBox^ gbSettings = gcnew System::Windows::Forms::GroupBox();
            gbSettings->Text = L"Настройки сканирования";
            gbSettings->Location = System::Drawing::Point(20, 20);
            gbSettings->Size = System::Drawing::Size(780, 160);
            gbSettings->Font = gcnew System::Drawing::Font("Segoe UI", 10, System::Drawing::FontStyle::Bold);
            gbSettings->BackColor = System::Drawing::Color::White;

            System::Windows::Forms::Label^ lblHost = gcnew System::Windows::Forms::Label();
            lblHost->Text = L"Целевой хост:";
            lblHost->Location = System::Drawing::Point(20, 35);
            lblHost->Size = System::Drawing::Size(140, 25);
            lblHost->TextAlign = ContentAlignment::MiddleRight;

            txtScanHost = gcnew System::Windows::Forms::TextBox();
            txtScanHost->Location = System::Drawing::Point(170, 33);
            txtScanHost->Size = System::Drawing::Size(200, 27);
            txtScanHost->Text = L"127.0.0.1";

            btnSelectFromNodes = gcnew System::Windows::Forms::Button();
            btnSelectFromNodes->Text = L"Выбрать из узлов";
            btnSelectFromNodes->Location = System::Drawing::Point(380, 32);
            btnSelectFromNodes->Size = System::Drawing::Size(160, 27);
            btnSelectFromNodes->BackColor = System::Drawing::Color::FromArgb(108, 117, 125);
            btnSelectFromNodes->ForeColor = System::Drawing::Color::White;
            btnSelectFromNodes->FlatStyle = FlatStyle::Flat;
            btnSelectFromNodes->Click += gcnew EventHandler(this, &Okno::btnSelectFromNodes_Click);

            System::Windows::Forms::Label^ lblStartPort = gcnew System::Windows::Forms::Label();
            lblStartPort->Text = L"Начальный порт:";
            lblStartPort->Location = System::Drawing::Point(20, 75);
            lblStartPort->Size = System::Drawing::Size(140, 25);
            lblStartPort->TextAlign = ContentAlignment::MiddleRight;

            numStartPort = gcnew System::Windows::Forms::NumericUpDown();
            numStartPort->Location = System::Drawing::Point(170, 73);
            numStartPort->Size = System::Drawing::Size(120, 27);
            numStartPort->Minimum = 1;
            numStartPort->Maximum = 65535;
            numStartPort->Value = 1;

            System::Windows::Forms::Label^ lblEndPort = gcnew System::Windows::Forms::Label();
            lblEndPort->Text = L"Конечный порт:";
            lblEndPort->Location = System::Drawing::Point(310, 75);
            lblEndPort->Size = System::Drawing::Size(140, 25);
            lblEndPort->TextAlign = ContentAlignment::MiddleRight;

            numEndPort = gcnew System::Windows::Forms::NumericUpDown();
            numEndPort->Location = System::Drawing::Point(460, 73);
            numEndPort->Size = System::Drawing::Size(120, 27);
            numEndPort->Minimum = 1;
            numEndPort->Maximum = 65535;
            numEndPort->Value = 100;

            System::Windows::Forms::Label^ lblProtocol = gcnew System::Windows::Forms::Label();
            lblProtocol->Text = L"Протоколы:";
            lblProtocol->Location = System::Drawing::Point(20, 115);
            lblProtocol->Size = System::Drawing::Size(140, 25);
            lblProtocol->TextAlign = ContentAlignment::MiddleRight;

            chkScanTCP = gcnew System::Windows::Forms::CheckBox();
            chkScanTCP->Text = L"TCP";
            chkScanTCP->Location = System::Drawing::Point(170, 117);
            chkScanTCP->Size = System::Drawing::Size(80, 25);
            chkScanTCP->Checked = true;

            chkScanUDP = gcnew System::Windows::Forms::CheckBox();
            chkScanUDP->Text = L"UDP";
            chkScanUDP->Location = System::Drawing::Point(270, 117);
            chkScanUDP->Size = System::Drawing::Size(80, 25);
            chkScanUDP->Checked = true;

            System::Windows::Forms::Label^ lblTimeout = gcnew System::Windows::Forms::Label();
            lblTimeout->Text = L"Таймаут(мс):";
            lblTimeout->Location = System::Drawing::Point(370, 115);
            lblTimeout->Size = System::Drawing::Size(110, 25);
            lblTimeout->TextAlign = ContentAlignment::MiddleRight;

            numTimeout = gcnew System::Windows::Forms::NumericUpDown();
            numTimeout->Location = System::Drawing::Point(490, 113);
            numTimeout->Size = System::Drawing::Size(100, 27);
            numTimeout->Minimum = 50;
            numTimeout->Maximum = 5000;
            numTimeout->Value = 100;

            gbSettings->Controls->Add(lblHost);
            gbSettings->Controls->Add(txtScanHost);
            gbSettings->Controls->Add(btnSelectFromNodes);
            gbSettings->Controls->Add(lblStartPort);
            gbSettings->Controls->Add(numStartPort);
            gbSettings->Controls->Add(lblEndPort);
            gbSettings->Controls->Add(numEndPort);
            gbSettings->Controls->Add(lblProtocol);
            gbSettings->Controls->Add(chkScanTCP);
            gbSettings->Controls->Add(chkScanUDP);
            gbSettings->Controls->Add(lblTimeout);
            gbSettings->Controls->Add(numTimeout);

            btnStartPortScan = gcnew System::Windows::Forms::Button();
            btnStartPortScan->Text = L"НАЧАТЬ СКАНИРОВАНИЕ";
            btnStartPortScan->Location = System::Drawing::Point(820, 55);
            btnStartPortScan->Size = System::Drawing::Size(220, 80);
            btnStartPortScan->BackColor = System::Drawing::Color::FromArgb(40, 167, 69);
            btnStartPortScan->ForeColor = System::Drawing::Color::White;
            btnStartPortScan->Font = gcnew System::Drawing::Font("Segoe UI", 12, System::Drawing::FontStyle::Bold);
            btnStartPortScan->FlatStyle = FlatStyle::Flat;
            btnStartPortScan->Click += gcnew EventHandler(this, &Okno::btnStartScan_Click);

            progressScan = gcnew System::Windows::Forms::ProgressBar();
            progressScan->Location = System::Drawing::Point(20, 200);
            progressScan->Size = System::Drawing::Size(1020, 25);

            lblScanStatus = gcnew System::Windows::Forms::Label();
            lblScanStatus->Text = L"Готов к сканированию";
            lblScanStatus->Location = System::Drawing::Point(20, 235);
            lblScanStatus->Size = System::Drawing::Size(600, 25);
            lblScanStatus->Font = gcnew System::Drawing::Font("Segoe UI", 9);

            System::Windows::Forms::Label^ lblResults = gcnew System::Windows::Forms::Label();
            lblResults->Text = L"РЕЗУЛЬТАТЫ СКАНИРОВАНИЯ:";
            lblResults->Location = System::Drawing::Point(20, 270);
            lblResults->Size = System::Drawing::Size(350, 25);
            lblResults->Font = gcnew System::Drawing::Font("Segoe UI", 10, System::Drawing::FontStyle::Bold);

            lbOpenPorts = gcnew System::Windows::Forms::ListBox();
            lbOpenPorts->Location = System::Drawing::Point(20, 300);
            lbOpenPorts->Size = System::Drawing::Size(1020, 400);
            lbOpenPorts->Font = gcnew System::Drawing::Font("Consolas", 10);
            lbOpenPorts->BackColor = System::Drawing::Color::FromArgb(30, 30, 30);
            lbOpenPorts->ForeColor = System::Drawing::Color::LightGreen;
            lbOpenPorts->IntegralHeight = false;

            this->tabPortScan->Controls->Add(gbSettings);
            this->tabPortScan->Controls->Add(btnStartPortScan);
            this->tabPortScan->Controls->Add(progressScan);
            this->tabPortScan->Controls->Add(lblScanStatus);
            this->tabPortScan->Controls->Add(lblResults);
            this->tabPortScan->Controls->Add(lbOpenPorts);
        }

        void InitializeLogTab()
        {
            this->tabLog->BackColor = System::Drawing::Color::FromArgb(240, 242, 245);

            System::Windows::Forms::Panel^ panelLogSettings = gcnew System::Windows::Forms::Panel();
            panelLogSettings->Location = System::Drawing::Point(10, 30);
            panelLogSettings->Size = System::Drawing::Size(960, 50);
            panelLogSettings->BackColor = System::Drawing::Color::White;
            panelLogSettings->BorderStyle = BorderStyle::FixedSingle;

            System::Windows::Forms::Label^ lblLogLevel = gcnew System::Windows::Forms::Label();
            lblLogLevel->Text = L"Уровень лога:";
            lblLogLevel->Location = System::Drawing::Point(10, 15);
            lblLogLevel->Size = System::Drawing::Size(100, 25);
            lblLogLevel->Font = gcnew System::Drawing::Font("Segoe UI", 9);

            cmbLogLevel = gcnew System::Windows::Forms::ComboBox();
            cmbLogLevel->Location = System::Drawing::Point(120, 13);
            cmbLogLevel->Size = System::Drawing::Size(120, 25);
            cmbLogLevel->Items->Add(L"Все события");
            cmbLogLevel->Items->Add(L"Только ошибки");
            cmbLogLevel->Items->Add(L"Только трафик");
            cmbLogLevel->Items->Add(L"Только узлы");
            cmbLogLevel->SelectedIndex = 0;

            chkLogTraffic = gcnew System::Windows::Forms::CheckBox();
            chkLogTraffic->Text = L"Логировать трафик";
            chkLogTraffic->Location = System::Drawing::Point(260, 15);
            chkLogTraffic->Size = System::Drawing::Size(150, 25);
            chkLogTraffic->Checked = true;

            lblLogPath = gcnew System::Windows::Forms::Label();
            lblLogPath->Text = L"Путь: " + currentLogPath;
            lblLogPath->Location = System::Drawing::Point(420, 15);
            lblLogPath->Size = System::Drawing::Size(400, 25);
            lblLogPath->Font = gcnew System::Drawing::Font("Segoe UI", 8);

            btnChooseLogPath = gcnew System::Windows::Forms::Button();
            btnChooseLogPath->Text = L"Выбрать папку";
            btnChooseLogPath->Location = System::Drawing::Point(830, 11);
            btnChooseLogPath->Size = System::Drawing::Size(110, 27);
            btnChooseLogPath->BackColor = System::Drawing::Color::FromArgb(108, 117, 125);
            btnChooseLogPath->ForeColor = System::Drawing::Color::White;
            btnChooseLogPath->FlatStyle = FlatStyle::Flat;
            btnChooseLogPath->Click += gcnew EventHandler(this, &Okno::btnChooseLogPath_Click);

            btnSaveLog = gcnew System::Windows::Forms::Button();
            btnSaveLog->Text = L"Экспорт";
            btnSaveLog->Location = System::Drawing::Point(850, 11);
            btnSaveLog->Size = System::Drawing::Size(100, 27);
            btnSaveLog->BackColor = System::Drawing::Color::FromArgb(40, 167, 69);
            btnSaveLog->ForeColor = System::Drawing::Color::White;
            btnSaveLog->FlatStyle = FlatStyle::Flat;
            btnSaveLog->Click += gcnew EventHandler(this, &Okno::btnSaveLog_Click);

            panelLogSettings->Controls->Add(lblLogLevel);
            panelLogSettings->Controls->Add(cmbLogLevel);
            panelLogSettings->Controls->Add(chkLogTraffic);
            panelLogSettings->Controls->Add(lblLogPath);
            panelLogSettings->Controls->Add(btnChooseLogPath);
            panelLogSettings->Controls->Add(btnSaveLog);

            rtbLog = gcnew System::Windows::Forms::RichTextBox();
            rtbLog->Location = System::Drawing::Point(10, 90);
            rtbLog->Size = System::Drawing::Size(960, 560);
            rtbLog->ReadOnly = true;
            rtbLog->Font = gcnew System::Drawing::Font("Consolas", 9);
            rtbLog->BackColor = System::Drawing::Color::FromArgb(30, 30, 30);
            rtbLog->ForeColor = System::Drawing::Color::LightGreen;

            System::Windows::Forms::Panel^ panelAlerts = gcnew System::Windows::Forms::Panel();
            panelAlerts->Location = System::Drawing::Point(10, 660);
            panelAlerts->Size = System::Drawing::Size(960, 45);
            panelAlerts->BackColor = System::Drawing::Color::White;
            panelAlerts->BorderStyle = BorderStyle::FixedSingle;

            chkSoundAlerts = gcnew System::Windows::Forms::CheckBox();
            chkSoundAlerts->Text = L"Звук";
            chkSoundAlerts->Location = System::Drawing::Point(10, 12);
            chkSoundAlerts->Size = System::Drawing::Size(80, 25);
            chkSoundAlerts->Checked = true;

            chkPopupAlerts = gcnew System::Windows::Forms::CheckBox();
            chkPopupAlerts->Text = L"Уведомления";
            chkPopupAlerts->Location = System::Drawing::Point(100, 12);
            chkPopupAlerts->Size = System::Drawing::Size(120, 25);
            chkPopupAlerts->Checked = true;

            btnClearEventLog = gcnew System::Windows::Forms::Button();
            btnClearEventLog->Text = L"Очистить";
            btnClearEventLog->Location = System::Drawing::Point(830, 8);
            btnClearEventLog->Size = System::Drawing::Size(110, 30);
            btnClearEventLog->BackColor = System::Drawing::Color::FromArgb(220, 53, 69);
            btnClearEventLog->ForeColor = System::Drawing::Color::White;
            btnClearEventLog->FlatStyle = FlatStyle::Flat;
            btnClearEventLog->Click += gcnew EventHandler(this, &Okno::btnClearLog_Click);

            panelAlerts->Controls->Add(chkSoundAlerts);
            panelAlerts->Controls->Add(chkPopupAlerts);
            panelAlerts->Controls->Add(btnClearEventLog);

            this->tabLog->Controls->Add(panelLogSettings);
            this->tabLog->Controls->Add(rtbLog);
            this->tabLog->Controls->Add(panelAlerts);
        }

        void InitializeInfoTab()
        {
            this->tabInfo->BackColor = System::Drawing::Color::FromArgb(240, 242, 245);
            this->tabInfo->Controls->Clear();

            System::Windows::Forms::Label^ lblInfoTitle = gcnew System::Windows::Forms::Label();
            lblInfoTitle->Text = L"Информация и советы";
            lblInfoTitle->Location = System::Drawing::Point(30, 30);
            lblInfoTitle->Size = System::Drawing::Size(450, 35);
            lblInfoTitle->Font = gcnew System::Drawing::Font("Segoe UI", 18, System::Drawing::FontStyle::Bold);
            lblInfoTitle->ForeColor = System::Drawing::Color::FromArgb(0, 120, 215);

            System::Windows::Forms::GroupBox^ gbAbout = gcnew System::Windows::Forms::GroupBox();
            gbAbout->Text = L"О программе";
            gbAbout->Location = System::Drawing::Point(30, 80);
            gbAbout->Size = System::Drawing::Size(500, 150);
            gbAbout->Font = gcnew System::Drawing::Font("Segoe UI", 11, System::Drawing::FontStyle::Bold);
            gbAbout->BackColor = System::Drawing::Color::White;

            System::Windows::Forms::Label^ lblAbout = gcnew System::Windows::Forms::Label();
            lblAbout->Text = L"Мониторинг и анализ сети v2.0\n\n"
                L"Программа для мониторинга сетевой активности,\n"
                L"отслеживания состояния узлов, сканирования портов\n"
                L"и анализа сетевых соединений.\n\n"
                L"Для связи с разработчиком: example@email.com";
            lblAbout->Location = System::Drawing::Point(20, 30);
            lblAbout->Size = System::Drawing::Size(460, 100);
            lblAbout->Font = gcnew System::Drawing::Font("Segoe UI", 10);
            gbAbout->Controls->Add(lblAbout);

            System::Windows::Forms::GroupBox^ gbNetworkStatus = gcnew System::Windows::Forms::GroupBox();
            gbNetworkStatus->Text = L"Статус сети";
            gbNetworkStatus->Location = System::Drawing::Point(30, 250);
            gbNetworkStatus->Size = System::Drawing::Size(500, 180);
            gbNetworkStatus->Font = gcnew System::Drawing::Font("Segoe UI", 11, System::Drawing::FontStyle::Bold);
            gbNetworkStatus->BackColor = System::Drawing::Color::White;

            lblNetworkStatus = gcnew System::Windows::Forms::Label();
            lblNetworkStatus->Text = L"Проверка...";
            lblNetworkStatus->Location = System::Drawing::Point(20, 30);
            lblNetworkStatus->Size = System::Drawing::Size(450, 30);
            lblNetworkStatus->Font = gcnew System::Drawing::Font("Segoe UI", 10);

            lblPublicIP = gcnew System::Windows::Forms::Label();
            lblPublicIP->Text = L"Внешний IP: Загрузка...";
            lblPublicIP->Location = System::Drawing::Point(20, 65);
            lblPublicIP->Size = System::Drawing::Size(450, 25);
            lblPublicIP->Font = gcnew System::Drawing::Font("Segoe UI", 10);

            lblUptime = gcnew System::Windows::Forms::Label();
            lblUptime->Text = L"Время работы: 0 ч 0 мин 0 сек";
            lblUptime->Location = System::Drawing::Point(20, 95);
            lblUptime->Size = System::Drawing::Size(450, 25);
            lblUptime->Font = gcnew System::Drawing::Font("Segoe UI", 10);

            lblCurrentTime = gcnew System::Windows::Forms::Label();
            lblCurrentTime->Text = DateTime::Now.ToString("dd.MM.yyyy HH:mm:ss");
            lblCurrentTime->Location = System::Drawing::Point(20, 125);
            lblCurrentTime->Size = System::Drawing::Size(450, 25);
            lblCurrentTime->Font = gcnew System::Drawing::Font("Segoe UI", 10);

            gbNetworkStatus->Controls->Add(lblNetworkStatus);
            gbNetworkStatus->Controls->Add(lblPublicIP);
            gbNetworkStatus->Controls->Add(lblUptime);
            gbNetworkStatus->Controls->Add(lblCurrentTime);

            System::Windows::Forms::GroupBox^ gbTips = gcnew System::Windows::Forms::GroupBox();
            gbTips->Text = L"Полезные советы";
            gbTips->Location = System::Drawing::Point(560, 80);
            gbTips->Size = System::Drawing::Size(650, 350);
            gbTips->Font = gcnew System::Drawing::Font("Segoe UI", 11, System::Drawing::FontStyle::Bold);
            gbTips->BackColor = System::Drawing::Color::White;

            System::Windows::Forms::ListBox^ lbTips = gcnew System::Windows::Forms::ListBox();
            lbTips->Location = System::Drawing::Point(20, 30);
            lbTips->Size = System::Drawing::Size(610, 290);
            lbTips->Font = gcnew System::Drawing::Font("Segoe UI", 10);
            lbTips->BackColor = System::Drawing::Color::FromArgb(250, 250, 250);
            lbTips->IntegralHeight = false;
            lbTips->Items->Add(L"Совет 1: Включите фоновый мониторинг для автоматической проверки узлов");
            lbTips->Items->Add(L"Совет 2: Используйте фильтр на вкладке 'Соединения' для поиска процессов");
            lbTips->Items->Add(L"Совет 3: При сканировании портов выбирайте небольшие диапазоны");
            lbTips->Items->Add(L"Совет 4: Журнал событий автоматически сохраняется в файл network_monitor.log");
            lbTips->Items->Add(L"Совет 5: Добавляйте важные узлы в список наблюдения для их отслеживания");
            lbTips->Items->Add(L"Совет 6: При проблемах со скоростью выполните 'lodctr /R' от имени администратора");
            lbTips->Items->Add(L"Совет 7: Выделение строк в таблицах сохраняется при обновлении данных");
            lbTips->Items->Add(L"Совет 8: Введите текст в поле фильтра и нажмите 'Применить' для поиска");
            lbTips->Items->Add(L"Совет 9: Узлы проверяются только по IPv4-адресам для надёжности");
            lbTips->Items->Add(L"Совет 10: На вкладке 'Главная' доступны быстрые действия");
            lbTips->Items->Add(L"Совет 11: Программа поддерживает импорт/экспорт списка узлов");
            lbTips->Items->Add(L"Совет 12: Нажмите 'Проверить все узлы' для ручного обновления статуса");

            gbTips->Controls->Add(lbTips);

            this->tabInfo->Controls->Add(lblInfoTitle);
            this->tabInfo->Controls->Add(gbAbout);
            this->tabInfo->Controls->Add(gbNetworkStatus);
            this->tabInfo->Controls->Add(gbTips);
        }

        void UpdateDashboardTab()
        {
            try
            {
                if (lbAvailableNodes != nullptr)
                {
                    int selectedIndex = lbAvailableNodes->SelectedIndex;
                    int currentTopIndex = lbAvailableNodes->TopIndex;

                    lbAvailableNodes->BeginUpdate();
                    lbAvailableNodes->Items->Clear();
                    int count = 0;
                    for each (NetworkNode ^ node in nodes)
                    {
                        if (node->IsAvailable)
                        {
                            String^ displayText = String::Format("{0} - {1} ms", node->IPAddress, node->ResponseTime);
                            lbAvailableNodes->Items->Add(displayText);
                            count++;
                        }
                    }
                    if (count == 0)
                    {
                        lbAvailableNodes->Items->Add("Нет доступных узлов");
                    }

                    if (currentTopIndex >= 0 && currentTopIndex < lbAvailableNodes->Items->Count)
                    {
                        lbAvailableNodes->TopIndex = currentTopIndex;
                    }
                    if (selectedIndex >= 0 && selectedIndex < lbAvailableNodes->Items->Count)
                    {
                        lbAvailableNodes->SelectedIndex = selectedIndex;
                    }
                    lbAvailableNodes->EndUpdate();
                }

                if (lbActiveConns != nullptr && allConnections != nullptr)
                {
                    int selectedIndex = lbActiveConns->SelectedIndex;
                    int currentTopIndex = lbActiveConns->TopIndex;

                    lbActiveConns->BeginUpdate();
                    lbActiveConns->Items->Clear();

                    int count = 0;
                    for each (ConnectionData ^ conn in allConnections)
                    {
                        if (count < 50)
                        {
                            String^ processDisplay = (conn->ProcessName != nullptr && conn->ProcessName != "")
                                ? conn->ProcessName
                                : (conn->Protocol == "UDP" ? "UDP Service" : "System");

                            if (processDisplay->Length > 20)
                                processDisplay = processDisplay->Substring(0, 17) + "...";

                            String^ displayText;

                            if (conn->Protocol == "UDP")
                            {
                                displayText = String::Format("UDP  {0}:{1}  [LISTENING]  [{2}]",
                                    conn->LocalAddr, conn->LocalPort, processDisplay);
                            }
                            else if (conn->Protocol == "TCP" && conn->State == "LISTENING")
                            {
                                displayText = String::Format("TCP  {0}:{1}  [LISTENING]  [{2}]",
                                    conn->LocalAddr, conn->LocalPort, processDisplay);
                            }
                            else
                            {
                                String^ remoteAddr = (String::IsNullOrEmpty(conn->RemoteAddr) || conn->RemoteAddr == "0.0.0.0")
                                    ? "*" : conn->RemoteAddr;
                                String^ remotePort = (conn->RemotePort == 0) ? "*" : conn->RemotePort.ToString();

                                displayText = String::Format("{0}  {1}:{2} -> {3}:{4}  [{5}]  [{6}]",
                                    conn->Protocol, conn->LocalAddr, conn->LocalPort,
                                    remoteAddr, remotePort, conn->State, processDisplay);
                            }

                            lbActiveConns->Items->Add(displayText);
                            count++;
                        }
                    }

                    if (allConnections->Count > 50)
                    {
                        lbActiveConns->Items->Add("... и еще " + (allConnections->Count - 50) + " соединений");
                    }

                    if (currentTopIndex >= 0 && currentTopIndex < lbActiveConns->Items->Count)
                    {
                        lbActiveConns->TopIndex = currentTopIndex;
                    }
                    if (selectedIndex >= 0 && selectedIndex < lbActiveConns->Items->Count)
                    {
                        lbActiveConns->SelectedIndex = selectedIndex;
                    }

                    lbActiveConns->EndUpdate();
                }

                if (lblTotalTraffic != nullptr)
                {
                    double totalMB = (totalDownloadBytes + totalUploadBytes) / (1024.0 * 1024.0);
                    lblTotalTraffic->Text = String::Format("{0:F2} MB", totalMB);
                }

                if (lblDashboardNodesCount != nullptr && nodes != nullptr)
                {
                    int availableCount = 0;
                    for each (NetworkNode ^ node in nodes)
                    {
                        if (node->IsAvailable) availableCount++;
                    }
                    lblDashboardNodesCount->Text = String::Format("{0} / {1}", availableCount, nodes->Count);
                }

                if (lblDashboardConnectionsCount != nullptr && filteredConnections != nullptr)
                {
                    lblDashboardConnectionsCount->Text = filteredConnections->Count.ToString();
                }

                if (lblDashboardUptime != nullptr)
                {
                    TimeSpan uptime = DateTime::Now - startTime;
                    lblDashboardUptime->Text = String::Format("Время работы: {0} ч {1} мин {2} сек",
                        uptime.Hours, uptime.Minutes, uptime.Seconds);
                }

                if (lblDashboardCurrentTime != nullptr)
                {
                    lblDashboardCurrentTime->Text = DateTime::Now.ToString("dd.MM.yyyy HH:mm:ss");
                }

                if (lblDashboardPublicIP != nullptr)
                {
                    lblDashboardPublicIP->Text = "Внешний IP: " + publicIP;
                }

                if (lblNetworkStatus != nullptr)
                {
                    UpdateNetworkStatus();
                }

                if (lblUptime != nullptr)
                {
                    TimeSpan uptime = DateTime::Now - startTime;
                    lblUptime->Text = String::Format("Время работы: {0} ч {1} мин {2} сек",
                        uptime.Hours, uptime.Minutes, uptime.Seconds);
                }

                if (lblCurrentTime != nullptr)
                {
                    lblCurrentTime->Text = DateTime::Now.ToString("dd.MM.yyyy HH:mm:ss");
                }

                if (lblPublicIP != nullptr)
                {
                    lblPublicIP->Text = "Внешний IP: " + publicIP;
                }
            }
            catch (Exception^ ex)
            {
                LogMessage("Ошибка в UpdateDashboardTab: " + ex->Message);
            }
        }

        void UpdateNetworkStatus()
        {
            try
            {
                array<NetworkInterface^>^ interfaces = NetworkInterface::GetAllNetworkInterfaces();
                if (interfaces != nullptr)
                {
                    bool isConnected = false;
                    for each (NetworkInterface ^ netIf in interfaces)
                    {
                        if (netIf != nullptr &&
                            netIf->OperationalStatus == OperationalStatus::Up &&
                            netIf->NetworkInterfaceType != NetworkInterfaceType::Loopback)
                        {
                            isConnected = true;
                            break;
                        }
                    }

                    if (lblNetworkStatus != nullptr)
                    {
                        if (isConnected)
                        {
                            lblNetworkStatus->Text = "Статус: ПОДКЛЮЧЕН";
                            lblNetworkStatus->ForeColor = System::Drawing::Color::FromArgb(40, 167, 69);
                        }
                        else
                        {
                            lblNetworkStatus->Text = "Статус: ОТКЛЮЧЕН";
                            lblNetworkStatus->ForeColor = System::Drawing::Color::FromArgb(220, 53, 69);
                        }
                    }
                }
                else
                {
                    if (lblNetworkStatus != nullptr)
                    {
                        lblNetworkStatus->Text = "Статус: НЕ УДАЛОСЬ ОПРЕДЕЛИТЬ";
                        lblNetworkStatus->ForeColor = System::Drawing::Color::FromArgb(255, 193, 7);
                    }
                }
            }
            catch (Exception^ ex)
            {
                LogMessage("Ошибка в UpdateNetworkStatus: " + ex->Message);
                if (lblNetworkStatus != nullptr)
                {
                    lblNetworkStatus->Text = "Статус: ОШИБКА";
                    lblNetworkStatus->ForeColor = System::Drawing::Color::FromArgb(220, 53, 69);
                }
            }
        }

        void UpdateRealConnections()
        {
            try
            {
                Monitor::Enter(trafficDataLock);
                allConnections->Clear();

                NativeConnectionData* tcpConns = NULL;
                DWORD tcpCount = 0;

                DWORD result = GetNativeTcpConnections(&tcpConns, &tcpCount);

                if (result == NO_ERROR && tcpConns != NULL)
                {
                    for (DWORD i = 0; i < tcpCount; i++)
                    {
                        NativeConnectionData* nativeConn = &tcpConns[i];

                        String^ localAddr = gcnew String(nativeConn->LocalAddr);
                        String^ remoteAddr = gcnew String(nativeConn->RemoteAddr);
                        String^ state = gcnew String(nativeConn->State);
                        String^ protocol = gcnew String(nativeConn->Protocol);

                        char procNameA[MAX_PATH] = { 0 };
                        GetProcessNameByPid(nativeConn->ProcessId, procNameA, MAX_PATH);
                        String^ processName = gcnew String(procNameA);

                        if (String::IsNullOrWhiteSpace(processName))
                            processName = "System";

                        ConnectionData^ conn = gcnew ConnectionData(
                            protocol, localAddr, nativeConn->LocalPort,
                            remoteAddr, nativeConn->RemotePort, state,
                            nativeConn->ProcessId, processName);
                        allConnections->Add(conn);
                    }
                    FreeNativeConnections(tcpConns);
                }

                NativeConnectionData* udpConns = NULL;
                DWORD udpCount = 0;

                result = GetNativeUdpConnections(&udpConns, &udpCount);

                if (result == NO_ERROR && udpConns != NULL)
                {
                    for (DWORD i = 0; i < udpCount; i++)
                    {
                        NativeConnectionData* nativeConn = &udpConns[i];

                        String^ localAddr = gcnew String(nativeConn->LocalAddr);
                        String^ protocol = gcnew String(nativeConn->Protocol);

                        char procNameA[MAX_PATH] = { 0 };
                        GetProcessNameByPid(nativeConn->ProcessId, procNameA, MAX_PATH);
                        String^ processName = gcnew String(procNameA);

                        if (String::IsNullOrWhiteSpace(processName))
                            processName = "System";

                        ConnectionData^ conn = gcnew ConnectionData(
                            protocol, localAddr, nativeConn->LocalPort,
                            "*", 0, "LISTENING",
                            nativeConn->ProcessId, processName);
                        allConnections->Add(conn);
                    }
                    FreeNativeConnections(udpConns);
                }

                LogMessage(String::Format("Обновлены соединения: Всего={0}", allConnections->Count));
            }
            catch (Exception^ ex)
            {
                LogMessage("Ошибка в UpdateRealConnections: " + ex->Message);
            }
            finally
            {
                Monitor::Exit(trafficDataLock);
            }
        }

        String^ GetProcessNameById(int pid)
        {
            if (processNameCache->ContainsKey(pid))
                return processNameCache[pid];

            try
            {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
                if (hProcess != NULL)
                {
                    TCHAR processName[MAX_PATH] = TEXT("<unknown>");
                    DWORD size = MAX_PATH;

                    if (QueryFullProcessImageName(hProcess, 0, processName, &size))
                    {
                        String^ fullPath = gcnew String(processName);
                        String^ name = System::IO::Path::GetFileName(fullPath);
                        processNameCache->Add(pid, name);
                        processPathCache->Add(pid, fullPath);
                        CloseHandle(hProcess);
                        return name;
                    }
                    CloseHandle(hProcess);
                }
            }
            catch (Exception^) {}

            processNameCache->Add(pid, "System");
            return "System";
        }

        void LoadDefaultConnections()
        {
            UpdateRealConnections();
            ApplyFilter();
        }

        int CountConnectionsByProtocol(String^ protocol)
        {
            int count = 0;
            for each (ConnectionData ^ conn in allConnections)
            {
                if (conn->Protocol == protocol) count++;
            }
            return count;
        }

        void ApplyFilter()
        {
            filteredConnections->Clear();

            String^ filterText = txtConnectionFilter->Text;
            String^ selectedProtocol = cmbProtocolFilter->SelectedItem->ToString();

            if (filterText == "Введите IP, порт или процесс...")
            {
                filterText = "";
            }

            for each (ConnectionData ^ conn in allConnections)
            {
                if (selectedProtocol != "Все" && conn->Protocol != selectedProtocol)
                {
                    continue;
                }

                if (!String::IsNullOrEmpty(filterText))
                {
                    bool match = false;

                    if (conn->LocalAddr->IndexOf(filterText, StringComparison::OrdinalIgnoreCase) >= 0)
                        match = true;
                    if (conn->RemoteAddr->IndexOf(filterText, StringComparison::OrdinalIgnoreCase) >= 0)
                        match = true;
                    if (conn->LocalPort.ToString()->IndexOf(filterText, StringComparison::OrdinalIgnoreCase) >= 0)
                        match = true;
                    if (conn->RemotePort.ToString()->IndexOf(filterText, StringComparison::OrdinalIgnoreCase) >= 0)
                        match = true;
                    if (conn->State->IndexOf(filterText, StringComparison::OrdinalIgnoreCase) >= 0)
                        match = true;
                    if (conn->Protocol->IndexOf(filterText, StringComparison::OrdinalIgnoreCase) >= 0)
                        match = true;
                    if (conn->ProcessName != nullptr && conn->ProcessName->IndexOf(filterText, StringComparison::OrdinalIgnoreCase) >= 0)
                        match = true;

                    if (!match)
                        continue;
                }

                filteredConnections->Add(conn);
            }
        }

        void Okno_Shown(Object^ sender, EventArgs^ e)
        {
            DrawSpeedChart();
        }

        void DrawSpeedChart()
        {
            if (pbDownloadChart == nullptr || pbUploadChart == nullptr) return;

            int width = pbDownloadChart->Width;
            int height = pbDownloadChart->Height;

            Bitmap^ bmpDownload = gcnew Bitmap(width, height);
            Bitmap^ bmpUpload = gcnew Bitmap(width, height);

            Graphics^ gDownload = Graphics::FromImage(bmpDownload);
            Graphics^ gUpload = Graphics::FromImage(bmpUpload);

            gDownload->Clear(Color::FromArgb(245, 245, 245));
            gUpload->Clear(Color::FromArgb(245, 245, 245));

            Pen^ gridPen = gcnew Pen(Color::FromArgb(220, 220, 220), 1);
            for (int i = 0; i <= 4; i++) {
                int y = i * height / 4;
                gDownload->DrawLine(gridPen, 0, y, width, y);
                gUpload->DrawLine(gridPen, 0, y, width, y);
            }

            if (downloadHistory->Count == CHART_POINTS_COUNT && uploadHistory->Count == CHART_POINTS_COUNT) {
                array<int>^ downloadData = downloadHistory->ToArray();
                array<int>^ uploadData = uploadHistory->ToArray();

                Pen^ downloadPen = gcnew Pen(Color::FromArgb(40, 167, 69), 2);
                Pen^ uploadPen = gcnew Pen(Color::FromArgb(220, 53, 69), 2);

                int step = width / CHART_POINTS_COUNT;
                int maxSpeed = 10240;

                for (int i = 0; i < CHART_POINTS_COUNT - 1; i++) {
                    int x1 = i * step;
                    int y1 = height - (downloadData[i] * height / maxSpeed);
                    int x2 = (i + 1) * step;
                    int y2 = height - (downloadData[i + 1] * height / maxSpeed);

                    y1 = Math::Max(0, Math::Min(height, y1));
                    y2 = Math::Max(0, Math::Min(height, y2));

                    gDownload->DrawLine(downloadPen, x1, y1, x2, y2);
                }

                for (int i = 0; i < CHART_POINTS_COUNT - 1; i++) {
                    int x1 = i * step;
                    int y1 = height - (uploadData[i] * height / maxSpeed);
                    int x2 = (i + 1) * step;
                    int y2 = height - (uploadData[i + 1] * height / maxSpeed);

                    y1 = Math::Max(0, Math::Min(height, y1));
                    y2 = Math::Max(0, Math::Min(height, y2));

                    gUpload->DrawLine(uploadPen, x1, y1, x2, y2);
                }
            }

            pbDownloadChart->Image = bmpDownload;
            pbUploadChart->Image = bmpUpload;
        }

        void btnRefreshDashboard_Click(Object^ sender, EventArgs^ e)
        {
            DrawSpeedChart();
            UpdateDashboardTab();
            LogMessage("Графики на главной вкладке обновлены вручную");
        }

        void btnQuickScan_Click(Object^ sender, EventArgs^ e)
        {
            if (tabControl1 != nullptr && tabPortScan != nullptr)
            {
                tabControl1->SelectedTab = tabPortScan;
                LogMessage("Быстрое действие: переход на вкладку сканера портов");
            }
        }

        void btnQuickPing_Click(Object^ sender, EventArgs^ e)
        {
            LogMessage("Быстрое действие: проверка всех узлов");

            if (nodes != nullptr)
            {
                for each (NetworkNode ^ node in nodes)
                {
                    CheckNodeAvailability(node);
                }
                UpdateNodesGrid();
                UpdateDashboardTab();

                MessageBox::Show("Проверка всех узлов завершена!", "Быстрая проверка",
                    MessageBoxButtons::OK, MessageBoxIcon::Information);
            }
        }

        void btnQuickExport_Click(Object^ sender, EventArgs^ e)
        {
            LogMessage("Быстрое действие: экспорт отчёта");

            SaveFileDialog^ saveDialog = gcnew SaveFileDialog();
            saveDialog->Title = "Сохранить отчёт";
            saveDialog->Filter = "HTML файлы (*.html)|*.html|Текстовые файлы (*.txt)|*.txt";
            saveDialog->DefaultExt = "html";
            saveDialog->FileName = "network_report_" + DateTime::Now.ToString("yyyyMMdd_HHmmss");

            if (saveDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK)
            {
                try
                {
                    StreamWriter^ writer = gcnew StreamWriter(saveDialog->FileName);

                    if (saveDialog->FileName->EndsWith(".html"))
                    {
                        writer->WriteLine("<html><head><title>Отчёт о сети</title>");
                        writer->WriteLine("<style>");
                        writer->WriteLine("body{font-family:'Segoe UI',Arial;margin:20px;background:#f0f2f5}");
                        writer->WriteLine("h1{color:#0078d7}");
                        writer->WriteLine(".stats{display:flex;gap:20px;margin:20px 0}");
                        writer->WriteLine(".stat-card{background:white;border-radius:10px;padding:15px;box-shadow:0 2px 4px rgba(0,0,0,0.1);min-width:150px}");
                        writer->WriteLine(".stat-value{font-size:28px;font-weight:bold;color:#0078d7}");
                        writer->WriteLine(".stat-label{color:#666;margin-top:5px}");
                        writer->WriteLine("table{border-collapse:collapse;width:100%;margin:15px 0}");
                        writer->WriteLine("th,td{border:1px solid #ddd;padding:10px;text-align:left}");
                        writer->WriteLine("th{background-color:#0078d7;color:white}");
                        writer->WriteLine(".success{color:#28a745;font-weight:bold}");
                        writer->WriteLine(".error{color:#dc3545;font-weight:bold}");
                        writer->WriteLine("</style>");
                        writer->WriteLine("</head><body>");
                        writer->WriteLine("<h1>Отчёт о сетевой активности</h1>");
                        writer->WriteLine("<p><strong>Дата создания:</strong> " + DateTime::Now.ToString() + "</p>");
                        writer->WriteLine("<p><strong>Внешний IP:</strong> " + publicIP + "</p>");
                        writer->WriteLine("<p><strong>Время работы программы:</strong> " + (DateTime::Now - startTime).ToString() + "</p>");
                        writer->WriteLine("<div class='stats'>");
                        writer->WriteLine("<div class='stat-card'><div class='stat-value'>" + nodes->Count + "</div><div class='stat-label'>Всего узлов</div></div>");

                        int availableNodes = 0;
                        for each (NetworkNode ^ node in nodes) if (node->IsAvailable) availableNodes++;
                        writer->WriteLine("<div class='stat-card'><div class='stat-value'>" + availableNodes + "</div><div class='stat-label'>Доступных узлов</div></div>");
                        writer->WriteLine("<div class='stat-card'><div class='stat-value'>" + filteredConnections->Count + "</div><div class='stat-label'>Активных соединений</div></div>");
                        writer->WriteLine("<div class='stat-card'><div class='stat-value'>" + String::Format("{0:F2}", (totalDownloadBytes + totalUploadBytes) / (1024.0 * 1024.0)) + " MB</div><div class='stat-label'>Всего трафика</div></div>");
                        writer->WriteLine("</div>");

                        writer->WriteLine("<h2>Наблюдаемые узлы</h2>");
                        writer->WriteLine("<table border='1'>");
                        writer->WriteLine("<tr><th>IP Адрес</th><th>Статус</th><th>Время отклика</th></tr>");
                        for each (NetworkNode ^ node in nodes)
                        {
                            String^ statusClass = node->IsAvailable ? "success" : "error";
                            String^ statusText = node->IsAvailable ? "Доступен" : "Недоступен";
                            String^ responseText = node->ResponseTime > 0 ? node->ResponseTime.ToString() + " ms" : "—";
                            writer->WriteLine("<tr>");
                            writer->WriteLine("    <td>" + node->IPAddress + "</td>");
                            writer->WriteLine("    <td class='" + statusClass + "'>" + statusText + "</td>");
                            writer->WriteLine("    <td>" + responseText + "</td>");
                            writer->WriteLine("</tr>");
                        }
                        writer->WriteLine("</table>");

                        writer->WriteLine("<h2>Активные соединения</h2>");
                        writer->WriteLine("<table border='1'>");
                        writer->WriteLine("<tr><th>Протокол</th><th>Локальный адрес</th><th>Порт</th><th>Удалённый адрес</th><th>Порт</th><th>Состояние</th><th>Процесс</th></tr>");
                        for each (ConnectionData ^ conn in filteredConnections)
                        {
                            writer->WriteLine("<tr>");
                            writer->WriteLine("    <td>" + conn->Protocol + "</td>");
                            writer->WriteLine("    <td>" + conn->LocalAddr + "</td>");
                            writer->WriteLine("    <td>" + conn->LocalPort + "</td>");
                            writer->WriteLine("    <td>" + conn->RemoteAddr + "</td>");
                            writer->WriteLine("    <td>" + conn->RemotePort + "</td>");
                            writer->WriteLine("    <td>" + conn->State + "</td>");
                            writer->WriteLine("    <td>" + (conn->ProcessName != nullptr ? conn->ProcessName : "N/A") + "</td>");
                            writer->WriteLine("</tr>");
                        }
                        writer->WriteLine("</table>");

                        writer->WriteLine("</body></html>");
                    }
                    else
                    {
                        writer->WriteLine("ОТЧЁТ О СЕТЕВОЙ АКТИВНОСТИ");
                        writer->WriteLine("========================");
                        writer->WriteLine("Дата: " + DateTime::Now.ToString());
                        writer->WriteLine("Внешний IP: " + publicIP);
                        writer->WriteLine("Время работы: " + (DateTime::Now - startTime).ToString());
                        writer->WriteLine("\nНАБЛЮДАЕМЫЕ УЗЛЫ:");
                        for each (NetworkNode ^ node in nodes)
                        {
                            writer->WriteLine(String::Format("  {0} - {1} ({2} ms)",
                                node->IPAddress, node->IsAvailable ? "Доступен" : "Недоступен", node->ResponseTime));
                        }
                        writer->WriteLine("\nАКТИВНЫЕ СОЕДИНЕНИЯ:");
                        for each (ConnectionData ^ conn in filteredConnections)
                        {
                            writer->WriteLine(String::Format("  {0} {1}:{2} -> {3}:{4} [{5}] [{6}]",
                                conn->Protocol, conn->LocalAddr, conn->LocalPort, conn->RemoteAddr, conn->RemotePort, conn->State,
                                conn->ProcessName != nullptr ? conn->ProcessName : "N/A"));
                        }
                        writer->WriteLine("\nСТАТИСТИКА ТРАФИКА:");
                        writer->WriteLine(String::Format("  Всего загружено: {0:F2} MB", totalDownloadBytes / (1024.0 * 1024.0)));
                        writer->WriteLine(String::Format("  Всего отправлено: {0:F2} MB", totalUploadBytes / (1024.0 * 1024.0)));
                    }

                    writer->Close();
                    MessageBox::Show("Отчёт успешно сохранён!", "Экспорт отчёта",
                        MessageBoxButtons::OK, MessageBoxIcon::Information);
                    LogMessage("Экспортирован отчёт: " + saveDialog->FileName);
                }
                catch (Exception^ ex)
                {
                    MessageBox::Show("Ошибка сохранения отчёта: " + ex->Message, "Ошибка",
                        MessageBoxButtons::OK, MessageBoxIcon::Error);
                }
            }
        }

        void btnResetTrafficStats_Click(Object^ sender, EventArgs^ e)
        {
            System::Windows::Forms::DialogResult result = MessageBox::Show("Вы уверены, что хотите сбросить всю статистику трафика?\n\nЗагружено: " +
                String::Format("{0:F2} MB", totalDownloadBytes / (1024.0 * 1024.0)) + "\nОтправлено: " +
                String::Format("{0:F2} MB", totalUploadBytes / (1024.0 * 1024.0)) + "\n\nСтатистика будет обнулена.",
                "Подтверждение сброса", MessageBoxButtons::YesNo, MessageBoxIcon::Warning);

            if (result == System::Windows::Forms::DialogResult::Yes)
            {
                totalDownloadBytes = 0;
                totalUploadBytes = 0;

                UpdateTrafficStats();
                UpdateDashboardTab();

                LogMessage("Статистика трафика сброшена пользователем");
                MessageBox::Show("Статистика трафика успешно сброшена!", "Сброс выполнен",
                    MessageBoxButtons::OK, MessageBoxIcon::Information);
            }
        }

        void btnExportTrafficReport_Click(Object^ sender, EventArgs^ e)
        {
            LogMessage("Экспорт отчёта по трафику");

            SaveFileDialog^ saveDialog = gcnew SaveFileDialog();
            saveDialog->Title = "Сохранить отчёт по трафику";
            saveDialog->Filter = "Текстовые файлы (*.txt)|*.txt|CSV файлы (*.csv)|*.csv|HTML файлы (*.html)|*.html";
            saveDialog->DefaultExt = "txt";
            saveDialog->FileName = "traffic_report_" + DateTime::Now.ToString("yyyyMMdd_HHmmss");

            if (saveDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK)
            {
                try
                {
                    StreamWriter^ writer = gcnew StreamWriter(saveDialog->FileName);

                    if (saveDialog->FileName->EndsWith(".html"))
                    {
                        writer->WriteLine("<html><head><title>Отчёт о трафике</title><style>");
                        writer->WriteLine("body{font-family:'Segoe UI',Arial;margin:20px;background:#f0f2f5}");
                        writer->WriteLine("h1{color:#0078d7}.stats{background:white;border-radius:10px;padding:20px}");
                        writer->WriteLine("</style></head><body>");
                        writer->WriteLine("<h1>Отчёт о сетевом трафике</h1>");
                        writer->WriteLine("<p><strong>Дата:</strong> " + DateTime::Now.ToString() + "</p>");
                        writer->WriteLine("<div class='stats'>");
                        writer->WriteLine("<h3>Общий трафик за сессию</h3>");
                        writer->WriteLine("<p>Загружено: " + String::Format("{0:F2} MB", totalDownloadBytes / (1024.0 * 1024.0)) + "</p>");
                        writer->WriteLine("<p>Отправлено: " + String::Format("{0:F2} MB", totalUploadBytes / (1024.0 * 1024.0)) + "</p>");
                        writer->WriteLine("<p>Итого: " + String::Format("{0:F2} MB", (totalDownloadBytes + totalUploadBytes) / (1024.0 * 1024.0)) + "</p>");
                        writer->WriteLine("</div></body></html>");
                    }
                    else
                    {
                        writer->WriteLine("========== ОТЧЁТ О СЕТЕВОМ ТРАФИКЕ ==========");
                        writer->WriteLine("Дата: " + DateTime::Now.ToString());
                        writer->WriteLine("");
                        writer->WriteLine("--- ОБЩИЙ ТРАФИК ЗА СЕССИЮ ---");
                        writer->WriteLine(String::Format("Загружено: {0:F2} MB", totalDownloadBytes / (1024.0 * 1024.0)));
                        writer->WriteLine(String::Format("Отправлено: {0:F2} MB", totalUploadBytes / (1024.0 * 1024.0)));
                        writer->WriteLine(String::Format("Итого: {0:F2} MB", (totalDownloadBytes + totalUploadBytes) / (1024.0 * 1024.0)));
                    }

                    writer->Close();
                    MessageBox::Show("Отчёт по трафику успешно сохранён!", "Экспорт",
                        MessageBoxButtons::OK, MessageBoxIcon::Information);
                }
                catch (Exception^ ex)
                {
                    MessageBox::Show("Ошибка сохранения отчёта: " + ex->Message, "Ошибка",
                        MessageBoxButtons::OK, MessageBoxIcon::Error);
                }
            }
        }

        void btnShowTrafficInfo_Click(Object^ sender, EventArgs^ e)
        {
            MessageBox::Show(
                "ПОЯСНЕНИЕ ПОКАЗАТЕЛЕЙ ТРАФИКА\n\n"
                "• KB/s — килобайт в секунду (1 KB = 1024 байта)\n"
                "• MB/s — мегабайт в секунду (1 MB = 1024 KB)\n\n"
                "Прогресс-бары отображают скорость до 10 MB/s.\n"
                "   При превышении бара шкала остаётся полной.\n\n"
                "Данные сбрасываются при перезапуске программы\n"
                "   или при нажатии кнопки 'Сбросить статистику'.",
                "Справка по трафику", MessageBoxButtons::OK, MessageBoxIcon::Information);
        }

        void UpdateTrafficStats()
        {
            if (lblTotalTraffic != nullptr)
            {
                double totalMB = (totalDownloadBytes + totalUploadBytes) / (1024.0 * 1024.0);
                lblTotalTraffic->Text = String::Format("{0:F2} MB", totalMB);
            }

            if (lblTotalDownload != nullptr)
                lblTotalDownload->Text = String::Format("{0:F2} MB", totalDownloadBytes / (1024.0 * 1024.0));
            if (lblTotalUpload != nullptr)
                lblTotalUpload->Text = String::Format("{0:F2} MB", totalUploadBytes / (1024.0 * 1024.0));

            if (lblActiveConnectionsCount != nullptr && allConnections != nullptr)
            {
                lblActiveConnectionsCount->Text = String::Format("Активных соединений: {0}", allConnections->Count);
            }

            if (lblDashboardConnectionsCount != nullptr && filteredConnections != nullptr)
            {
                lblDashboardConnectionsCount->Text = filteredConnections->Count.ToString();
            }
        }

        void dgvNodes_RowEnter(Object^ sender, DataGridViewCellEventArgs^ e)
        {
            if (e->RowIndex >= 0 && e->RowIndex < nodes->Count)
            {
                selectedNodeIndex = e->RowIndex;
            }
        }

        void GetPublicIP()
        {
            try
            {
                System::Net::WebClient^ client = gcnew System::Net::WebClient();
                client->Headers->Add("User-Agent", "Mozilla/5.0");
                String^ ip = client->DownloadString("https://api.ipify.org");
                publicIP = ip->Trim();
                if (lblPublicIP != nullptr)
                    lblPublicIP->Text = "Внешний IP: " + publicIP;
                if (lblDashboardPublicIP != nullptr)
                    lblDashboardPublicIP->Text = "Внешний IP: " + publicIP;
                LogMessage("Внешний IP: " + publicIP);
            }
            catch (Exception^ ex)
            {
                publicIP = "Не удалось определить";
                if (lblPublicIP != nullptr)
                    lblPublicIP->Text = "Внешний IP: Не удалось определить";
                if (lblDashboardPublicIP != nullptr)
                    lblDashboardPublicIP->Text = "Внешний IP: Не удалось определить";
                LogMessage("Ошибка получения внешнего IP: " + ex->Message);
            }
        }

        void timerClock_Tick(Object^ sender, EventArgs^ e)
        {
            try
            {
                UpdateDashboardTab();
            }
            catch (Exception^ ex)
            {
                LogMessage("Ошибка в timerClock_Tick: " + ex->Message);
            }
        }

        void btnImportNodes_Click(Object^ sender, EventArgs^ e)
        {
            OpenFileDialog^ openDialog = gcnew OpenFileDialog();
            openDialog->Title = "Импорт узлов из файла";
            openDialog->Filter = "Текстовые файлы (*.txt)|*.txt|Все файлы (*.*)|*.*";

            if (openDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK)
            {
                try
                {
                    array<String^>^ lines = File::ReadAllLines(openDialog->FileName);
                    int added = 0;
                    for each (String ^ line in lines)
                    {
                        String^ trimmed = line->Trim();
                        if (!String::IsNullOrEmpty(trimmed) && !trimmed->StartsWith("#"))
                        {
                            AddNode(trimmed);
                            added++;
                        }
                    }
                    MessageBox::Show(String::Format("Импортировано узлов: {0}", added), "Импорт", MessageBoxButtons::OK, MessageBoxIcon::Information);
                    LogMessage(String::Format("Импортировано {0} узлов из файла", added));
                    UpdateDashboardTab();
                }
                catch (Exception^ ex)
                {
                    MessageBox::Show("Ошибка импорта: " + ex->Message, "Ошибка", MessageBoxButtons::OK, MessageBoxIcon::Error);
                }
            }
        }

        void btnExportNodes_Click(Object^ sender, EventArgs^ e)
        {
            SaveFileDialog^ saveDialog = gcnew SaveFileDialog();
            saveDialog->Title = "Экспорт узлов";
            saveDialog->Filter = "Текстовые файлы (*.txt)|*.txt";
            saveDialog->DefaultExt = "txt";
            saveDialog->FileName = "nodes_export_" + DateTime::Now.ToString("yyyyMMdd_HHmmss");

            if (saveDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK)
            {
                try
                {
                    StreamWriter^ writer = gcnew StreamWriter(saveDialog->FileName);
                    writer->WriteLine("# Список наблюдаемых узлов");
                    writer->WriteLine("# Экспортировано: " + DateTime::Now.ToString());
                    writer->WriteLine("#");
                    for each (NetworkNode ^ node in nodes)
                    {
                        writer->WriteLine(node->IPAddress);
                    }
                    writer->Close();
                    MessageBox::Show(String::Format("Экспортировано узлов: {0}", nodes->Count), "Экспорт", MessageBoxButtons::OK, MessageBoxIcon::Information);
                    LogMessage(String::Format("Экспортировано {0} узлов в файл", nodes->Count));
                }
                catch (Exception^ ex)
                {
                    MessageBox::Show("Ошибка экспорта: " + ex->Message, "Ошибка", MessageBoxButtons::OK, MessageBoxIcon::Error);
                }
            }
        }

        void txtFilter_Enter(Object^ sender, EventArgs^ e)
        {
            TextBox^ txt = (TextBox^)sender;
            if (txt->Text == "Введите IP, порт или процесс...")
            {
                txt->Text = "";
                txt->ForeColor = System::Drawing::Color::Black;
            }
        }

        void txtFilter_Leave(Object^ sender, EventArgs^ e)
        {
            TextBox^ txt = (TextBox^)sender;
            if (String::IsNullOrWhiteSpace(txt->Text))
            {
                txt->Text = "Введите IP, порт или процесс...";
                txt->ForeColor = System::Drawing::Color::Gray;
            }
        }

        void btnFilterConnections_Click(Object^ sender, EventArgs^ e)
        {
            ApplyFilter();
            UpdateConnectionsGrid();
            LogMessage(String::Format("Применён фильтр: текст='{0}', протокол='{1}'",
                txtConnectionFilter->Text, cmbProtocolFilter->SelectedItem->ToString()));
        }

        void StartTrafficMonitoring()
        {
            try
            {
                timerTraffic = gcnew System::Windows::Forms::Timer();
                timerTraffic->Interval = 1000;
                timerTraffic->Tick += gcnew EventHandler(this, &Okno::timerTraffic_Tick);
                timerTraffic->Start();

                timerConnections = gcnew System::Windows::Forms::Timer();
                timerConnections->Interval = 10000;
                timerConnections->Tick += gcnew EventHandler(this, &Okno::timerConnections_Tick);
                timerConnections->Start();

                LogMessage("Мониторинг трафика запущен");
            }
            catch (Exception^ ex)
            {
                LogMessage("Ошибка запуска мониторинга трафика: " + ex->Message);
            }
        }

        void SaveConnectionsScrollState()
        {
            if (dgvConnections == nullptr) return;

            try
            {
                if (dgvConnections->Rows->Count > 0)
                {
                    connectionsScrollState->FirstDisplayedRowIndex = dgvConnections->FirstDisplayedScrollingRowIndex;
                    connectionsScrollState->HorizontalOffset = dgvConnections->HorizontalScrollingOffset;

                    if (dgvConnections->SelectedRows != nullptr && dgvConnections->SelectedRows->Count > 0)
                    {
                        connectionsScrollState->SelectedRowIndex = dgvConnections->SelectedRows[0]->Index;
                    }
                    else
                    {
                        connectionsScrollState->SelectedRowIndex = -1;
                    }
                }
            }
            catch (Exception^ ex)
            {
                LogMessage("Ошибка сохранения состояния скролла: " + ex->Message);
            }
        }

        void RestoreConnectionsScrollState()
        {
            if (dgvConnections == nullptr) return;

            try
            {
                if (connectionsScrollState->FirstDisplayedRowIndex >= 0 &&
                    connectionsScrollState->FirstDisplayedRowIndex < dgvConnections->Rows->Count)
                {
                    dgvConnections->FirstDisplayedScrollingRowIndex = connectionsScrollState->FirstDisplayedRowIndex;
                }

                if (connectionsScrollState->HorizontalOffset >= 0)
                {
                    dgvConnections->HorizontalScrollingOffset = connectionsScrollState->HorizontalOffset;
                }

                if (connectionsScrollState->SelectedRowIndex >= 0 &&
                    connectionsScrollState->SelectedRowIndex < dgvConnections->Rows->Count)
                {
                    dgvConnections->ClearSelection();
                    dgvConnections->Rows[connectionsScrollState->SelectedRowIndex]->Selected = true;
                }
            }
            catch (Exception^ ex)
            {
                LogMessage("Ошибка восстановления состояния скролла: " + ex->Message);
            }
        }

        void timerTraffic_Tick(Object^ sender, EventArgs^ e)
        {
            try
            {
                long long downloadSpeedKB = 0;
                long long uploadSpeedKB = 0;

                GetNetworkSpeed(downloadSpeedKB, uploadSpeedKB);

                long long smoothedDownSpeed = GetSmoothedSpeed(downloadSpeedBuffer, downloadSpeedKB);
                long long smoothedUpSpeed = GetSmoothedSpeed(uploadSpeedBuffer, uploadSpeedKB);

                if (lblDownloadSpeed != nullptr)
                {
                    lblDownloadSpeed->Text = String::Format("{0:F2} KB/s ({1:F2} MB/s)",
                        smoothedDownSpeed, smoothedDownSpeed / 1024.0);
                }
                if (lblUploadSpeed != nullptr)
                {
                    lblUploadSpeed->Text = String::Format("{0:F2} KB/s ({1:F2} MB/s)",
                        smoothedUpSpeed, smoothedUpSpeed / 1024.0);
                }

                if (progressBarDownload != nullptr)
                {
                    int downloadProgress = (int)smoothedDownSpeed;
                    if (downloadProgress > progressBarDownload->Maximum)
                        downloadProgress = progressBarDownload->Maximum;
                    progressBarDownload->Value = downloadProgress;
                }
                if (progressBarUpload != nullptr)
                {
                    int uploadProgress = (int)smoothedUpSpeed;
                    if (uploadProgress > progressBarUpload->Maximum)
                        uploadProgress = progressBarUpload->Maximum;
                    progressBarUpload->Value = uploadProgress;
                }

                if (downloadHistory->Count >= CHART_POINTS_COUNT) downloadHistory->Dequeue();
                if (uploadHistory->Count >= CHART_POINTS_COUNT) uploadHistory->Dequeue();

                downloadHistory->Enqueue((int)smoothedDownSpeed);
                uploadHistory->Enqueue((int)smoothedUpSpeed);

                DrawSpeedChart();
                UpdateTrafficStats();
                UpdateDashboardTab();

                trafficCounter++;
                if (trafficCounter >= 30)
                {
                    trafficCounter = 0;
                    LogMessage(String::Format("[ТРАФИК] Всего загружено: {0:F2} MB, Отправлено: {1:F2} MB, Текущая скорость: {2:F1} KB/s {3:F1} KB/s",
                        totalDownloadBytes / (1024.0 * 1024.0), totalUploadBytes / (1024.0 * 1024.0),
                        smoothedDownSpeed, smoothedUpSpeed));
                }
            }
            catch (Exception^ ex)
            {
                LogMessage("Ошибка в timerTraffic_Tick: " + ex->Message);
            }
        }

        void LogTrafficActivity(double downloadSpeed, double uploadSpeed)
        {
            if (!chkLogTraffic->Checked) return;

            if (downloadSpeed > 100 || uploadSpeed > 100)
            {
                LogMessage(String::Format("[ТРАФИК] Загрузка: {0:F2} KB/s, Отдача: {1:F2} KB/s | Всего: {2:F2} MB / {3:F2} MB",
                    downloadSpeed, uploadSpeed, totalDownloadBytes / (1024.0 * 1024.0), totalUploadBytes / (1024.0 * 1024.0)));
            }
        }

        void LogTrafficSummary()
        {
            LogMessage(String::Format("[ИТОГО ТРАФИК ЗА СЕССИЮ] Загружено: {0:F2} MB, Отправлено: {1:F2} MB",
                totalDownloadBytes / (1024.0 * 1024.0), totalUploadBytes / (1024.0 * 1024.0)));
        }

        void UpdateConnectionsGrid()
        {
            try
            {
                if (dgvConnections == nullptr) return;

                SaveConnectionsScrollState();

                if (dgvConnections->Columns->Count == 0)
                {
                    dgvConnections->Columns->Add("Protocol", "Протокол");
                    dgvConnections->Columns->Add("LocalAddr", "Локальный адрес");
                    dgvConnections->Columns->Add("LocalPort", "Порт");
                    dgvConnections->Columns->Add("RemoteAddr", "Удаленный адрес");
                    dgvConnections->Columns->Add("RemotePort", "Порт");
                    dgvConnections->Columns->Add("State", "Состояние");
                    dgvConnections->Columns->Add("ProcessName", "Процесс");

                    dgvConnections->Columns[0]->Width = 80;
                    dgvConnections->Columns[1]->Width = 140;
                    dgvConnections->Columns[2]->Width = 80;
                    dgvConnections->Columns[3]->Width = 160;
                    dgvConnections->Columns[4]->Width = 80;
                    dgvConnections->Columns[5]->Width = 120;
                    dgvConnections->Columns[6]->Width = 150;
                }

                dgvConnections->Rows->Clear();

                Dictionary<String^, int>^ connectionCount = gcnew Dictionary<String^, int>();
                for each (ConnectionData ^ conn in filteredConnections)
                {
                    String^ procName = conn->ProcessName != nullptr ? conn->ProcessName : "N/A";
                    if (connectionCount->ContainsKey(procName))
                        connectionCount[procName]++;
                    else
                        connectionCount->Add(procName, 1);
                }

                for each (ConnectionData ^ conn in filteredConnections)
                {
                    int rowIndex = dgvConnections->Rows->Add(conn->Protocol, conn->LocalAddr, conn->LocalPort,
                        conn->RemoteAddr, conn->RemotePort, conn->State,
                        conn->ProcessName != nullptr ? conn->ProcessName : "N/A");

                    String^ procName = conn->ProcessName != nullptr ? conn->ProcessName : "N/A";
                    int count = connectionCount[procName];

                    if (count >= 20)
                        dgvConnections->Rows[rowIndex]->DefaultCellStyle->BackColor = Color::FromArgb(255, 200, 200);
                    else if (count >= 10)
                        dgvConnections->Rows[rowIndex]->DefaultCellStyle->BackColor = Color::FromArgb(255, 230, 150);
                    else if (conn->State == "ESTABLISHED")
                        dgvConnections->Rows[rowIndex]->DefaultCellStyle->BackColor = Color::FromArgb(200, 255, 200);
                }

                RestoreConnectionsScrollState();

                UpdateTrafficStats();
                UpdateDashboardTab();
            }
            catch (Exception^ ex)
            {
                LogMessage("Ошибка в UpdateConnectionsGrid: " + ex->Message);
            }
        }

        long long PingHost(String^ host)
        {
            try
            {
                String^ cleanHost = host->Split(' ')[0];

                IPHostEntry^ hostEntry = Dns::GetHostEntry(cleanHost);
                IPAddress^ ipAddress = nullptr;

                for each (IPAddress ^ addr in hostEntry->AddressList)
                {
                    if (addr->AddressFamily == System::Net::Sockets::AddressFamily::InterNetwork)
                    {
                        ipAddress = addr;
                        break;
                    }
                }

                if (ipAddress == nullptr)
                {
                    return -1;
                }

                System::Net::NetworkInformation::Ping^ ping = gcnew System::Net::NetworkInformation::Ping();
                System::Net::NetworkInformation::PingReply^ reply = ping->Send(ipAddress, 3000);

                long long result = -1;

                if (reply != nullptr && reply->Status == System::Net::NetworkInformation::IPStatus::Success)
                {
                    result = reply->RoundtripTime;
                }

                delete ping;
                return result;
            }
            catch (Exception^)
            {
                return -1;
            }
        }

        void CheckNodeAvailability(NetworkNode^ node)
        {
            if (node == nullptr) return;

            long long responseTime = PingHost(node->IPAddress);
            bool wasAvailable = node->IsAvailable;

            node->IsAvailable = (responseTime >= 0);
            node->ResponseTime = responseTime >= 0 ? responseTime : 0;
            node->LastCheck = DateTime::Now;
            node->PacketLoss = node->IsAvailable ? 0 : 100;

            if (!wasAvailable && node->IsAvailable)
            {
                LogMessage(String::Format("Узел {0} стал ДОСТУПЕН ({1} мс)", node->IPAddress, responseTime));
                ShowAlert(String::Format("Узел {0} доступен", node->IPAddress));
            }
            else if (wasAvailable && !node->IsAvailable)
            {
                LogMessage(String::Format("Узел {0} стал НЕДОСТУПЕН", node->IPAddress));
                ShowAlert(String::Format("Узел {0} недоступен!", node->IPAddress));
            }
        }

        void AddNode(String^ address)
        {
            if (String::IsNullOrEmpty(address)) return;

            for each (NetworkNode ^ existingNode in nodes)
            {
                if (existingNode->IPAddress->Equals(address, StringComparison::OrdinalIgnoreCase))
                {
                    LogMessage(String::Format("Узел {0} уже существует в списке", address));
                    return;
                }
            }

            NetworkNode^ node = gcnew NetworkNode();
            node->IPAddress = address;
            nodes->Add(node);

            CheckNodeAvailability(node);
            UpdateNodesGrid();
            SaveNodesToFile();
            UpdateDashboardTab();
        }

        void UpdateNodesGrid()
        {
            try
            {
                if (dgvNodes == nullptr) return;

                int previousSelectedIndex = selectedNodeIndex;

                if (dgvNodes->Columns->Count == 0)
                {
                    dgvNodes->Columns->Add("IP", "IP Адрес / Хост");
                    dgvNodes->Columns->Add("Status", "Статус");
                    dgvNodes->Columns->Add("Response", "Время отклика (мс)");
                    dgvNodes->Columns->Add("Loss", "Потери пакетов (%)");
                    dgvNodes->Columns->Add("LastCheck", "Последняя проверка");
                    dgvNodes->Columns[0]->Width = 200;
                    dgvNodes->Columns[1]->Width = 100;
                    dgvNodes->Columns[2]->Width = 130;
                    dgvNodes->Columns[3]->Width = 100;
                    dgvNodes->Columns[4]->Width = 150;
                }

                dgvNodes->Rows->Clear();

                for each (NetworkNode ^ node in nodes)
                {
                    String^ status = node->IsAvailable ? "ДОСТУПЕН" : "НЕДОСТУПЕН";
                    String^ ipDisplay = !String::IsNullOrEmpty(node->IPAddress) ? node->IPAddress : "ОШИБКА: адрес пуст";

                    String^ responseDisplay = "Проверка...";
                    if (node->ResponseTime >= 0 && node->IsAvailable)
                    {
                        if (node->ResponseTime > 0)
                            responseDisplay = node->ResponseTime.ToString() + " ms";
                        else
                            responseDisplay = "< 1 ms";
                    }
                    else if (!node->IsAvailable && node->LastCheck != DateTime::MinValue)
                    {
                        responseDisplay = "Таймаут";
                    }

                    String^ lossDisplay = node->PacketLoss.ToString() + "%";
                    String^ lastCheckDisplay = node->LastCheck.ToString("HH:mm:ss");
                    if (node->LastCheck == DateTime::MinValue)
                        lastCheckDisplay = "не проверялся";

                    int rowIndex = dgvNodes->Rows->Add(
                        ipDisplay, status, responseDisplay, lossDisplay, lastCheckDisplay
                    );

                    if (node->IsAvailable)
                        dgvNodes->Rows[rowIndex]->DefaultCellStyle->BackColor = System::Drawing::Color::LightGreen;
                    else
                        dgvNodes->Rows[rowIndex]->DefaultCellStyle->BackColor = System::Drawing::Color::LightCoral;
                }

                if (previousSelectedIndex >= 0 && previousSelectedIndex < dgvNodes->Rows->Count)
                {
                    dgvNodes->ClearSelection();
                    dgvNodes->Rows[previousSelectedIndex]->Selected = true;
                }
                else if (dgvNodes->Rows->Count > 0)
                {
                    dgvNodes->Rows[0]->Selected = true;
                    selectedNodeIndex = 0;
                }

                if (lblMonitoredNodesCount != nullptr && nodes != nullptr)
                {
                    int availableCount = 0;
                    for each (NetworkNode ^ node in nodes) if (node->IsAvailable) availableCount++;
                    lblMonitoredNodesCount->Text = String::Format("{0} / {1}", availableCount, nodes->Count);
                }

                UpdateDashboardTab();
            }
            catch (Exception^ ex)
            {
                LogMessage("Ошибка в UpdateNodesGrid: " + ex->Message);
            }
        }

        void LoadDefaultNodes()
        {
            AddNode("8.8.8.8");
            AddNode("1.1.1.1");
            AddNode("ya.ru");
            AddNode("google.com");
        }

        void SaveNodesToFile()
        {
            try
            {
                XmlTextWriter^ writer = gcnew XmlTextWriter(configFilePath, System::Text::Encoding::UTF8);
                writer->Formatting = Formatting::Indented;
                writer->WriteStartDocument();
                writer->WriteStartElement("NetworkNodes");
                for each (NetworkNode ^ node in nodes)
                {
                    writer->WriteStartElement("Node");
                    writer->WriteElementString("IPAddress", node->IPAddress);
                    writer->WriteEndElement();
                }
                writer->WriteEndElement();
                writer->WriteEndDocument();
                writer->Close();
            }
            catch (Exception^ ex)
            {
                LogMessage("Ошибка сохранения конфигурации: " + ex->Message);
            }
        }

        void LoadNodesFromFile()
        {
            if (!File::Exists(configFilePath)) return;
            try
            {
                nodes->Clear();
                XmlDocument^ doc = gcnew XmlDocument();
                doc->Load(configFilePath);
                XmlNodeList^ nodeList = doc->SelectNodes("//Node");
                for each (XmlNode ^ xmlNode in nodeList)
                {
                    XmlNode^ ipNode = xmlNode->SelectSingleNode("IPAddress");
                    if (ipNode != nullptr)
                    {
                        String^ ipAddress = ipNode->InnerText;
                        if (!String::IsNullOrEmpty(ipAddress))
                        {
                            NetworkNode^ node = gcnew NetworkNode();
                            node->IPAddress = ipAddress;
                            nodes->Add(node);
                        }
                    }
                }
                LogMessage(String::Format("Загружено узлов из конфигурации: {0}", nodes->Count));
            }
            catch (Exception^ ex)
            {
                LogMessage("Ошибка загрузки конфигурации: " + ex->Message);
            }
        }

        void LogMessage(String^ message)
        {
            String^ timestamp = DateTime::Now.ToString("yyyy-MM-dd HH:mm:ss");
            String^ logEntry = String::Format("[{0}] {1}\r\n", timestamp, message);

            if (rtbLog != nullptr)
            {
                if (rtbLog->InvokeRequired)
                {
                    rtbLog->Invoke(gcnew Action<String^>(this, &Okno::LogMessage), message);
                }
                else
                {
                    rtbLog->AppendText(logEntry);
                    rtbLog->ScrollToCaret();
                }
            }

            if (logWriter != nullptr)
            {
                try
                {
                    Monitor::Enter(logLock);
                    logWriter->Write(logEntry);
                    Monitor::Exit(logLock);
                }
                catch (Exception^)
                {
                    Monitor::Exit(logLock);
                }
            }
        }

        void ShowAlert(String^ message)
        {
            if (chkPopupAlerts->Checked)
            {
                if (this->InvokeRequired)
                {
                    this->Invoke(gcnew Action<String^>(this, &Okno::ShowAlert), message);
                }
                else
                {
                    NotifyIcon^ notifyIcon = gcnew NotifyIcon();
                    notifyIcon->Icon = System::Drawing::SystemIcons::Information;
                    notifyIcon->Visible = true;
                    notifyIcon->ShowBalloonTip(3000, "Мониторинг сети", message, ToolTipIcon::Info);
                }
            }
            if (chkSoundAlerts->Checked)
            {
                System::Media::SystemSounds::Beep->Play();
            }
        }

        void timerConnections_Tick(Object^ sender, EventArgs^ e)
        {
            UpdateRealConnections();
            ApplyFilter();
            UpdateConnectionsGrid();
        }

        void timerNodes_Tick(Object^ sender, EventArgs^ e)
        {
            if (chkBackgroundMonitoring->Checked && nodes != nullptr)
            {
                for each (NetworkNode ^ node in nodes)
                {
                    CheckNodeAvailability(node);
                }
                UpdateNodesGrid();
                UpdateDashboardTab();
            }
        }

        void btnRefreshConnections_Click(Object^ sender, EventArgs^ e)
        {
            UpdateRealConnections();
            ApplyFilter();
            UpdateConnectionsGrid();
        }

        void chkBackgroundMonitoring_Changed(Object^ sender, EventArgs^ e)
        {
            if (chkBackgroundMonitoring->Checked)
            {
                if (timerNodes != nullptr) timerNodes->Start();
                LogMessage("Фоновый мониторинг включен");
            }
            else
            {
                if (timerNodes != nullptr) timerNodes->Stop();
                LogMessage("Фоновый мониторинг выключен");
            }
        }

        void txtNewNodeIP_Enter(Object^ sender, EventArgs^ e)
        {
            if (txtNewNodeIP->Text == "Введите IP или домен...")
            {
                txtNewNodeIP->Text = "";
                txtNewNodeIP->ForeColor = System::Drawing::Color::Black;
            }
        }

        void txtNewNodeIP_Leave(Object^ sender, EventArgs^ e)
        {
            if (String::IsNullOrWhiteSpace(txtNewNodeIP->Text))
            {
                txtNewNodeIP->Text = "Введите IP или домен...";
                txtNewNodeIP->ForeColor = System::Drawing::Color::Gray;
            }
        }

        void btnAddNode_Click(Object^ sender, EventArgs^ e)
        {
            String^ address = txtNewNodeIP->Text->Trim();
            if (!String::IsNullOrEmpty(address) && address != "Введите IP или домен...")
            {
                AddNode(address);
                LogMessage(String::Format("Добавлен узел в мониторинг: {0}", address));
                txtNewNodeIP->Clear();
                txtNewNodeIP_Leave(sender, e);
            }
        }

        void btnRemoveNode_Click(Object^ sender, EventArgs^ e)
        {
            if (dgvNodes != nullptr && dgvNodes->SelectedRows->Count > 0)
            {
                int index = dgvNodes->SelectedRows[0]->Index;
                if (index < nodes->Count)
                {
                    String^ nodeIP = nodes[index]->IPAddress;
                    nodes->RemoveAt(index);

                    if (selectedNodeIndex >= nodes->Count)
                        selectedNodeIndex = nodes->Count - 1;
                    if (selectedNodeIndex < 0 && nodes->Count > 0)
                        selectedNodeIndex = 0;

                    UpdateNodesGrid();
                    SaveNodesToFile();
                    LogMessage(String::Format("Удален узел из мониторинга: {0}", nodeIP));
                }
            }
            else
            {
                MessageBox::Show("Пожалуйста, выберите узел для удаления.", "Удаление узла", MessageBoxButtons::OK, MessageBoxIcon::Information);
            }
        }

        void btnPingSelected_Click(Object^ sender, EventArgs^ e)
        {
            if (dgvNodes != nullptr && dgvNodes->SelectedRows->Count > 0)
            {
                int index = dgvNodes->SelectedRows[0]->Index;
                if (index < nodes->Count)
                {
                    NetworkNode^ node = nodes[index];
                    long long responseTime = PingHost(node->IPAddress);
                    if (responseTime >= 0)
                    {
                        MessageBox::Show(String::Format("Результат пинга для узла {0}\n\nУзел доступен\nВремя отклика: {1} мс",
                            node->IPAddress, responseTime), "Результат пинга", MessageBoxButtons::OK, MessageBoxIcon::Information);
                        LogMessage(String::Format("Ручной пинг узла {0}: доступен ({1} мс)", node->IPAddress, responseTime));

                        node->IsAvailable = true;
                        node->ResponseTime = responseTime;
                        node->LastCheck = DateTime::Now;
                        node->PacketLoss = 0;
                        UpdateNodesGrid();
                    }
                    else
                    {
                        MessageBox::Show(String::Format("Результат пинга для узла {0}\n\nУзел НЕ ДОСТУПЕН",
                            node->IPAddress), "Результат пинга", MessageBoxButtons::OK, MessageBoxIcon::Warning);
                        LogMessage(String::Format("Ручной пинг узла {0}: недоступен", node->IPAddress));

                        node->IsAvailable = false;
                        node->ResponseTime = 0;
                        node->LastCheck = DateTime::Now;
                        node->PacketLoss = 100;
                        UpdateNodesGrid();
                    }
                }
            }
            else
            {
                MessageBox::Show("Пожалуйста, выберите узел для пинга.", "Пинг узла", MessageBoxButtons::OK, MessageBoxIcon::Information);
            }
        }

        void btnScanSelectedNode_Click(Object^ sender, EventArgs^ e)
        {
            if (dgvNodes != nullptr && dgvNodes->SelectedRows->Count > 0)
            {
                int index = dgvNodes->SelectedRows[0]->Index;
                if (index < nodes->Count)
                {
                    NetworkNode^ node = nodes[index];
                    String^ cleanHost = node->IPAddress->Split(' ')[0];
                    if (tabControl1 != nullptr && tabPortScan != nullptr)
                        tabControl1->SelectedTab = tabPortScan;
                    if (txtScanHost != nullptr)
                        txtScanHost->Text = cleanHost;
                    LogMessage(String::Format("Выбран узел для сканирования портов: {0}", cleanHost));
                }
            }
            else
            {
                MessageBox::Show("Пожалуйста, выберите узел для сканирования портов.", "Сканирование портов", MessageBoxButtons::OK, MessageBoxIcon::Information);
            }
        }

        void btnSelectFromNodes_Click(Object^ sender, EventArgs^ e)
        {
            if (nodes == nullptr || nodes->Count == 0)
            {
                MessageBox::Show("Список наблюдаемых узлов пуст. Добавьте узлы на вкладке 'Узлы'.",
                    "Нет узлов", MessageBoxButtons::OK, MessageBoxIcon::Information);
                return;
            }

            Form^ selectForm = gcnew Form();
            selectForm->Text = "Выберите узел для сканирования";
            selectForm->Size = System::Drawing::Size(400, 500);
            selectForm->StartPosition = FormStartPosition::CenterParent;
            selectForm->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
            selectForm->MaximizeBox = false;
            selectForm->MinimizeBox = false;

            ListBox^ listBox = gcnew ListBox();
            listBox->Dock = DockStyle::Fill;
            listBox->Font = gcnew System::Drawing::Font("Segoe UI", 10);

            for each (NetworkNode ^ node in nodes)
            {
                String^ displayText = node->IPAddress;
                if (node->IsAvailable)
                {
                    displayText += " (доступен, " + node->ResponseTime.ToString() + " ms)";
                }
                else
                {
                    displayText += " (недоступен)";
                }
                listBox->Items->Add(displayText);
            }

            if (listBox->Items->Count > 0)
                listBox->SelectedIndex = 0;

            Panel^ buttonPanel = gcnew Panel();
            buttonPanel->Height = 50;
            buttonPanel->Dock = DockStyle::Bottom;

            Button^ btnOK = gcnew Button();
            btnOK->Text = "Выбрать";
            btnOK->Location = System::Drawing::Point(100, 10);
            btnOK->Size = System::Drawing::Size(100, 30);
            btnOK->DialogResult = System::Windows::Forms::DialogResult::OK;
            btnOK->BackColor = System::Drawing::Color::FromArgb(40, 167, 69);
            btnOK->ForeColor = System::Drawing::Color::White;
            btnOK->FlatStyle = FlatStyle::Flat;

            Button^ btnCancel = gcnew Button();
            btnCancel->Text = "Отмена";
            btnCancel->Location = System::Drawing::Point(210, 10);
            btnCancel->Size = System::Drawing::Size(100, 30);
            btnCancel->DialogResult = System::Windows::Forms::DialogResult::Cancel;
            btnCancel->BackColor = System::Drawing::Color::FromArgb(108, 117, 125);
            btnCancel->ForeColor = System::Drawing::Color::White;
            btnCancel->FlatStyle = FlatStyle::Flat;

            buttonPanel->Controls->Add(btnOK);
            buttonPanel->Controls->Add(btnCancel);

            selectForm->Controls->Add(listBox);
            selectForm->Controls->Add(buttonPanel);

            if (selectForm->ShowDialog() == System::Windows::Forms::DialogResult::OK)
            {
                if (listBox->SelectedItem != nullptr)
                {
                    String^ selectedDisplay = listBox->SelectedItem->ToString();
                    int spaceIndex = selectedDisplay->IndexOf(' ');
                    String^ selectedNode = (spaceIndex > 0) ? selectedDisplay->Substring(0, spaceIndex) : selectedDisplay;
                    if (txtScanHost != nullptr)
                        txtScanHost->Text = selectedNode;
                    LogMessage(String::Format("Выбран узел для сканирования из списка: {0}", selectedNode));
                }
            }

            delete selectForm;
        }

        void btnStartScan_Click(Object^ sender, EventArgs^ e)
        {
            if (isScanning)
            {
                MessageBox::Show("Сканирование уже выполняется!", "Предупреждение", MessageBoxButtons::OK, MessageBoxIcon::Warning);
                return;
            }

            if (!chkScanTCP->Checked && !chkScanUDP->Checked)
            {
                MessageBox::Show("Выберите хотя бы один протокол!", "Ошибка", MessageBoxButtons::OK, MessageBoxIcon::Error);
                return;
            }

            String^ host = txtScanHost->Text;
            if (String::IsNullOrWhiteSpace(host))
            {
                MessageBox::Show("Введите хост для сканирования!", "Ошибка", MessageBoxButtons::OK, MessageBoxIcon::Error);
                return;
            }

            int startPort = (int)numStartPort->Value;
            int endPort = (int)numEndPort->Value;

            if (startPort > endPort)
            {
                MessageBox::Show("Начальный порт не может быть больше конечного!", "Ошибка", MessageBoxButtons::OK, MessageBoxIcon::Error);
                return;
            }

            int portsCount = endPort - startPort + 1;
            if (portsCount > 1000)
            {
                System::Windows::Forms::DialogResult result = MessageBox::Show(String::Format("Вы собираетесь сканировать {0} портов. Это может занять много времени. Продолжить?", portsCount),
                    "Подтверждение", MessageBoxButtons::YesNo, MessageBoxIcon::Question);
                if (result != System::Windows::Forms::DialogResult::Yes)
                    return;
            }

            isScanning = true;
            btnStartPortScan->Enabled = false;
            btnStartPortScan->Text = "Сканирование...";
            progressScan->Maximum = 100;
            progressScan->Value = 0;
            lbOpenPorts->Items->Clear();
            lblScanStatus->Text = "Сканирование запущено...";

            ScanParameters^ params = gcnew ScanParameters();
            params->Host = host;
            params->StartPort = startPort;
            params->EndPort = endPort;
            params->ScanTCP = chkScanTCP->Checked;
            params->ScanUDP = chkScanUDP->Checked;
            params->Timeout = (int)numTimeout->Value;

            Thread^ scanThread = gcnew Thread(gcnew ParameterizedThreadStart(this, &Okno::ScanPortsAsync));
            scanThread->Start(params);
        }

        void ScanPortsAsync(Object^ parameters)
        {
            ScanParameters^ params = (ScanParameters^)parameters;
            ScanPorts(params->Host, params->StartPort, params->EndPort, params->ScanTCP, params->ScanUDP, params->Timeout);
        }

        void ScanPorts(String^ host, int startPort, int endPort, bool scanTCP, bool scanUDP, int timeout)
        {
            try
            {
                IPAddress^ ipAddress = nullptr;
                try
                {
                    ipAddress = Dns::GetHostEntry(host)->AddressList[0];
                }
                catch (Exception^ ex)
                {
                    LogMessage("Ошибка разрешения имени хоста: " + ex->Message);
                    UpdateScanStatus("Ошибка: не удалось разрешить имя хоста");
                    return;
                }

                int openPortsCount = 0;
                int totalScans = (scanTCP ? (endPort - startPort + 1) : 0) + (scanUDP ? (endPort - startPort + 1) : 0);
                int currentScan = 0;

                LogMessage(String::Format("Начато сканирование хоста {0} ({1}) порты {2}-{3}", host, ipAddress->ToString(), startPort, endPort));

                if (scanTCP)
                {
                    for (int port = startPort; port <= endPort; port++)
                    {
                        if (!isScanning) break;
                        try
                        {
                            TcpClient^ client = gcnew TcpClient();
                            IAsyncResult^ ar = client->BeginConnect(ipAddress, port, nullptr, nullptr);
                            if (ar->AsyncWaitHandle->WaitOne(timeout, false))
                            {
                                if (client->Connected)
                                {
                                    AddOpenTCPPort(port);
                                    openPortsCount++;
                                    client->EndConnect(ar);
                                }
                            }
                            client->Close();
                        }
                        catch (Exception^) {}

                        currentScan++;
                        if (totalScans > 0)
                        {
                            UpdateScanProgress((currentScan * 100) / totalScans);
                        }
                    }
                }

                if (scanUDP)
                {
                    for (int port = startPort; port <= endPort; port++)
                    {
                        if (!isScanning) break;
                        try
                        {
                            UdpClient^ udpClient = gcnew UdpClient();
                            udpClient->Client->ReceiveTimeout = timeout;
                            udpClient->Connect(ipAddress, port);
                            array<Byte>^ sendBytes = System::Text::Encoding::ASCII->GetBytes("ping");
                            udpClient->Send(sendBytes, sendBytes->Length);
                            AddOpenUDPPort(port);
                            openPortsCount++;
                            udpClient->Close();
                        }
                        catch (Exception^) {}

                        currentScan++;
                        if (totalScans > 0)
                        {
                            UpdateScanProgress((currentScan * 100) / totalScans);
                        }
                    }
                }

                LogMessage(String::Format("Сканирование портов {0} ({1}:{2}) завершено. Найдено открытых портов: {3}", host, startPort, endPort, openPortsCount));
                UpdateScanStatus(String::Format("Сканирование завершено. Найдено открытых портов: {0}", openPortsCount));
            }
            catch (Exception^ ex)
            {
                LogMessage("Ошибка сканирования: " + ex->Message);
                UpdateScanStatus("Ошибка сканирования: " + ex->Message);
            }
            finally
            {
                isScanning = false;
                EnableScanButton();
            }
        }

        void AddOpenTCPPort(int port)
        {
            if (this->InvokeRequired)
            {
                this->Invoke(gcnew Action<int>(this, &Okno::AddOpenTCPPort), port);
                return;
            }
            if (lbOpenPorts != nullptr)
                lbOpenPorts->Items->Add(String::Format("[TCP] Порт {0} - ОТКРЫТ", port));
        }

        void AddOpenUDPPort(int port)
        {
            if (this->InvokeRequired)
            {
                this->Invoke(gcnew Action<int>(this, &Okno::AddOpenUDPPort), port);
                return;
            }
            if (lbOpenPorts != nullptr)
                lbOpenPorts->Items->Add(String::Format("[UDP] Порт {0} - ОТКРЫТ (возможно)", port));
        }

        void UpdateScanProgress(int percent)
        {
            if (this->InvokeRequired)
            {
                this->Invoke(gcnew Action<int>(this, &Okno::UpdateScanProgress), percent);
                return;
            }
            if (progressScan != nullptr)
                progressScan->Value = Math::Min(percent, 100);
            if (lblScanStatus != nullptr)
                lblScanStatus->Text = String::Format("Прогресс сканирования: {0}%", percent);
        }

        void UpdateScanStatus(String^ status)
        {
            if (this->InvokeRequired)
            {
                this->Invoke(gcnew Action<String^>(this, &Okno::UpdateScanStatus), status);
                return;
            }
            if (lblScanStatus != nullptr)
                lblScanStatus->Text = status;
        }

        void EnableScanButton()
        {
            if (this->InvokeRequired)
            {
                this->Invoke(gcnew Action(this, &Okno::EnableScanButton));
                return;
            }
            if (btnStartPortScan != nullptr)
            {
                btnStartPortScan->Enabled = true;
                btnStartPortScan->Text = "НАЧАТЬ СКАНИРОВАНИЕ";
            }
            if (lblScanStatus != nullptr && (lblScanStatus->Text == "Сканирование запущено..." || lblScanStatus->Text->StartsWith("Прогресс")))
            {
                lblScanStatus->Text = "Сканирование завершено";
            }
        }

        void btnClearLog_Click(Object^ sender, EventArgs^ e)
        {
            if (rtbLog != nullptr)
                rtbLog->Clear();
            LogMessage("Журнал событий очищен пользователем");
        }

        void btnChooseLogPath_Click(Object^ sender, EventArgs^ e)
        {
            FolderBrowserDialog^ folderDialog = gcnew FolderBrowserDialog();
            folderDialog->Description = "Выберите папку для сохранения логов";
            if (folderDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK)
            {
                String^ newLogPath = folderDialog->SelectedPath + "\\network_monitor.log";
                try
                {
                    if (logWriter != nullptr)
                    {
                        logWriter->Flush();
                        logWriter->Close();
                    }
                    currentLogPath = newLogPath;
                    logWriter = gcnew StreamWriter(currentLogPath, true);
                    logWriter->AutoFlush = true;
                    if (lblLogPath != nullptr)
                        lblLogPath->Text = "Путь: " + currentLogPath;
                    LogMessage("Лог-файл перемещен в: " + currentLogPath);
                }
                catch (Exception^ ex)
                {
                    MessageBox::Show("Ошибка: " + ex->Message, "Ошибка", MessageBoxButtons::OK, MessageBoxIcon::Error);
                }
            }
        }

        void btnSaveLog_Click(Object^ sender, EventArgs^ e)
        {
            SaveFileDialog^ saveDialog = gcnew SaveFileDialog();
            saveDialog->Title = "Сохранить журнал";
            saveDialog->Filter = "Текстовые файлы (*.txt)|*.txt|Лог-файлы (*.log)|*.log";
            saveDialog->DefaultExt = "txt";
            saveDialog->FileName = "log_export_" + DateTime::Now.ToString("yyyyMMdd_HHmmss");

            if (saveDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK)
            {
                try
                {
                    StreamWriter^ writer = gcnew StreamWriter(saveDialog->FileName);
                    if (rtbLog != nullptr)
                        writer->Write(rtbLog->Text);
                    writer->Close();
                    MessageBox::Show("Журнал сохранен!", "Сохранение", MessageBoxButtons::OK, MessageBoxIcon::Information);
                    LogMessage("Журнал экспортирован в: " + saveDialog->FileName);
                }
                catch (Exception^ ex)
                {
                    MessageBox::Show("Ошибка: " + ex->Message, "Ошибка", MessageBoxButtons::OK, MessageBoxIcon::Error);
                }
            }
        }
    };
}

#endif