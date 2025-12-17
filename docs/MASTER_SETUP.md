# Classroom Control - Master Application Setup Guide

This guide details the installation, configuration, and operation of the Classroom Control Master Application (`cms_master`).

## 1. System Requirements

The Master Application is the central console for managing classroom computers. Scaling depends on the number of connected clients.

| Component | Minimum Specification | Recommended (30+ Clients) |
| :--- | :--- | :--- |
| **OS** | Windows 8+, Ubuntu 18.04+, macOS 10.14+ | Windows 10/11, Ubuntu 20.04+, macOS 12+ |
| **CPU** | Dual-core 2.0GHz | Quad-core 2.5GHz+ |
| **RAM** | 2GB | 4GB + (10MB per active client) |
| **Disk** | 1GB free space | 10GB+ (SSD recommended for screenshot cache) |
| **Network** | 100 Mbps Ethernet | 1 Gigabit Ethernet |
| **Display** | 1366x768 resolution | 1920x1080 (Dual monitor support available) |

---

## 2. Installation

### Windows
1.  **Download** the installer: `cms_master_installer.exe`.
2.  **Run as Administrator**: Right-click the file and select "Run as administrator".
3.  **Destination**: Choose install location (Default: `C:\Program Files\ClassroomControlMaster`).
4.  **Startup**: Select "Launch automatically at startup" if this is a dedicated teacher station.
5.  **Firewall**: The installer will automatically add an exception for TCP port 5555.
6.  **Finish**: Reboot is optional but recommended.

### Linux (Debian/Ubuntu)
1.  **Install Package**:
    ```bash
    sudo apt-get update
    sudo apt-get install classroom-control-master
    ```
2.  **Configuration**: The installer creates `/etc/classroom-control/master.conf`.
3.  **Data Directory**: By default, data is stored in `/var/lib/classroom-control`. Ensure the user has permissions if running manually.

### macOS
1.  **Download** the disk image: `cms_master.dmg`.
2.  **Install**: Open the DMG and drag `Classroom Control Master` to the `Applications` folder.
3.  **First Run**:
    *   Open "Classroom Control Master" from Applications.
    *   macOS may verify the developer signature. Click "Open".
    *   The setup wizard will configure initial preferences.

---

## 3. Initial Configuration

Upon first launch, the **Configuration Wizard** will appear.

### Network Settings
*   **Server Port**: Default is `5555`. Ensure this port is open on your network firewall.
*   **Interface**: Select the network adapter connected to the classroom LAN (e.g., `eth0` or `Ethernet`).
*   **Max Clients**: Set the soft limit for concurrent connections (e.g., `40`).

### Performance & Security
*   **Screenshot Cache**:
    *   **Size**: Default `500 MB`. Increase for longer history.
    *   **Location**: Choose a fast drive (SSD).
*   **Encryption**:
    *   [x] Enable SSL/TLS (Recommended)
    *   **Certificate**: Auto-generate self-signed or import an institution certificate.
*   **Authentication**:
    *   **Local Password**: Set an admin password for the Master console.
    *   **LDAP Integration**: (Optional) Configure under "Advanced" to use school directory logic.

---

## 4. Add Clients to Master

Clients can be added in three ways via the "Client Manager" tab:

1.  **Network Discovery (Recommended)**:
    *   Click **Scan Network**.
    *   The Master will broadcast a discovery packet.
    *   Select found clients and click **Add**.
2.  **Manual Add**:
    *   Click **Add Client**.
    *   Enter **IP Address** and **Host Name**.
3.  **Import CSV**:
    *   Prepare a CSV file with columns: `Hostname,IP,MAC`.
    *   Click **Import List** -> Select file.

### Grouping
*   **Labs**: Create groups (e.g., "Lab 101", "Library") and drag clients into them for easier management.

---

## 5. User Accounts & Permissions

Access to the Master Application can be restricted.

*   **Administrator**: Full access to global settings, network config, and user management.
*   **Teacher**: access to monitoring, screen lock, and remote control. Cannot change network ports or potential security settings.
*   **Guest/Sub**: View-only mode (optional).

### Active Directory / LDAP
To configure central authentication:
1.  Go to **Settings -> Users -> LDAP**.
2.  Enter Server URL (e.g., `ldap://dc01.school.local`).
3.  Bind DN and Password.
4.  Map "Teachers" group to application role.

---

## 6. Security Setup

### Encryption (SSL/TLS)
If enabled, you must distribute the CA certificate to clients if using a self-signed certificate, or ensure the server certificate is trusted.
1.  **Generate**: Settings -> Security -> Generate Certificate.
2.  **Export**: Export the public key (`master.crt`).
3.  **Deploy**: Install `master.crt` on Client machines in the Trusted Root store (Windows) or appropriate Keychain (macOS).

### Firewall Rules
Ensure the following traffic is allowed:
*   **Inbound (Master)**: TCP 5555 (Command/Control), UDP 5556 (Discovery - optional).
*   **Outbound (Master)**: TCP 80/443 (Updates), TCP to Clients (Reverse connections if configured).

### Password Policies
*   Enforce a strong password for the Master Console startup to prevent unauthorized student access.

---

## 7. Testing

Before first class, perform this validation checklist:

- [ ] **Client Connection**: Verify at least one client appears "Online" (Green indicator).
- [ ] **Screenshot**: Click a client thumbnail. Does the screen update?
- [ ] **Lock/Unlock**: Select a client, press **Lock**. Verify client input is disabled. Press **Unlock**.
- [ ] **Broadcast**: Send a text message "Test" to a client.
- [ ] **Power Control**: Test "Shutdown" on a non-critical test machine.

---

## 8. Daily Operation

### Startup
1.  Launch **Classroom Control Master**.
2.  Login with Teacher credentials.
3.  Wait for client thumbnails to populate (usually 10-30 seconds).

### Common Tasks
*   **View Client List**: Main Dashboard shows grid view. Toggle to list view for details (IP, OS).
*   **Monitor**: Double-click a thumbnail for **Remote View** (High-res stream).
*   **Control**: Right-click a client for context menu (Lock, Reboot, Send File).

### Performance Monitoring
Check the status bar for:
*   **CPU Usage**: High usage may indicate too many incoming streams. Reduce thumbnail refresh rate in Settings.
*   **Network**: Check "Bandwidth" meter. If saturated, lower screenshot quality.

---

## Deployment Checklist

- [ ] Network subnet allows broadcast (if using discovery).
- [ ] Static IP assigned to Master Station (Recommended).
- [ ] Firewall exceptions active on Master and Clients.
- [ ] Antivirus exclusions added for `cms_client` and `cms_master`.
- [ ] Admin password recorded securely.

## Advanced Topics

### VPN Configuration
For remote teaching, ensure the VPN allows routing to the client subnet. The Master must be reachable by Clients if using reverse-connect, or Clients must be reachable by Master for direct connect.

### Database Maintenance
The SQLite database (`cms_data.db`) stores logs and attendance.
*   **Backup**: Copy the file weekly.
*   **Pruning**: Settings -> Database -> "Auto-delete logs older than 30 days".
