## 🔑 Google Drive API Setup

To use Google Drive integration, you need to create credentials and set them in a `.env` file.

### Step 1 – Enable the API
1. Go to [Google Cloud Console](https://console.cloud.google.com/).  
2. Create a new project (or use an existing one).  
3. Navigate to **APIs & Services → Library**, search for **Google Drive API**, and enable it.  

### Step 2 – Configure OAuth Consent Screen
1. Go to **APIs & Services → OAuth consent screen**.  
2. Select **External** (or Internal if using a Google Workspace).  
3. Enter an **App name**, **support email**, and add the required Drive scope:  
   - `https://www.googleapis.com/auth/drive.file`  
4. Save and continue.  

### Step 3 – Create OAuth Credentials
1. Go to **APIs & Services → Credentials → Create Credentials → OAuth Client ID**.  
2. Choose **Desktop app** (or Web app if you have a redirect URL).  
3. Copy the generated **Client ID** and **Client Secret**.  

### Step 4 – Get a Refresh Token
1. Open [OAuth 2.0 Playground](https://developers.google.com/oauthplayground/).  
2. Click the ⚙️ gear icon → check **Use your own OAuth credentials** and paste your Client ID & Secret.  
3. Select the scope `https://www.googleapis.com/auth/drive.file`.  
4. Click **Authorize APIs** and then **Exchange authorization code for tokens**.  
5. Copy the **Refresh Token**.  

### Step 5 – Fill in the `.env` file
Create a `.env` file in your project root with the following:

```env
GOOGLE_DRIVE_CLIENT_ID=your_client_id_here
GOOGLE_DRIVE_CLIENT_SECRET=your_client_secret_here
GOOGLE_DRIVE_REFRESH_TOKEN=your_refresh_token_here
NOTIFICATION_EMAIL=your_email@example.com
