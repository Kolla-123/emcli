# Pushing EMCLI to GitHub

This guide walks you through creating a new repository on GitHub and pushing the local commits.

## Prerequisites

1. **GitHub Account**: Sign up at https://github.com if you don't have one
2. **Git Installed**: Verify with `git --version`
3. **Authenticated**: Set up GitHub authentication (SSH key or personal access token)

## Step-by-Step Instructions

### Step 1: Create a New Repository on GitHub

1. Go to [GitHub.com](https://github.com)
2. Click **+** (top right) → **New repository**
3. Fill in the details:
   - **Repository name**: `emcli` (or your preferred name)
   - **Description**: "Embedded Command Line Interface - Lightweight CLI framework for embedded systems"
   - **Public** or **Private**: Choose your preference
   - **Initialize repository**: Leave unchecked (we already have commits)
4. Click **Create repository**

### Step 2: Add Remote Repository

After creating the repository, GitHub will show you setup commands. Run:

```bash
cd c:\Users\Saidurga\OneDrive\Documents\emcli
git remote add origin https://github.com/YOUR_USERNAME/emcli.git
```

Or if using SSH (recommended):
```bash
git remote add origin git@github.com:YOUR_USERNAME/emcli.git
```

Replace `YOUR_USERNAME` with your actual GitHub username.

### Step 3: Verify Remote

Confirm the remote was added correctly:

```bash
git remote -v
```

You should see:
```
origin  https://github.com/YOUR_USERNAME/emcli.git (fetch)
origin  https://github.com/YOUR_USERNAME/emcli.git (push)
```

### Step 4: Push to GitHub

Push your commits to the remote repository:

```bash
git branch -M main
git push -u origin main
```

This:
- Renames `master` branch to `main` (GitHub convention)
- `-u` sets the upstream tracking (future pushes just need `git push`)
- Uploads all commits to GitHub

### Step 5: Verify on GitHub

1. Go back to your GitHub repository page
2. Refresh the page
3. You should see:
   - All your commits in the commit history
   - README.md displayed on the repository home
   - File tree showing all source files

## Git Configuration for Future Pushes

### Option A: HTTPS (Password + Token)

```bash
git remote set-url origin https://github.com/YOUR_USERNAME/emcli.git
```

You'll be prompted for:
- Username: Your GitHub username
- Password: A personal access token (not your actual password)

**To create a personal access token**:
1. Go to GitHub Settings → Developer settings → Personal access tokens
2. Click "Generate new token"
3. Grant `repo` scope
4. Copy the token and use as password

### Option B: SSH (Recommended)

```bash
# Generate SSH key (if you don't have one)
ssh-keygen -t ed25519 -C "your.email@example.com"

# Add to SSH agent
ssh-add ~/.ssh/id_ed25519

# Add public key to GitHub:
# 1. Copy your public key: cat ~/.ssh/id_ed25519.pub
# 2. Go to GitHub Settings → SSH and GPG keys
# 3. Click "New SSH key"
# 4. Paste and save

# Set remote to SSH
git remote set-url origin git@github.com:YOUR_USERNAME/emcli.git
```

## Common Commands for Future Development

### Push Changes
```bash
git add .
git commit -m "your message"
git push
```

### Pull Latest Changes
```bash
git pull origin main
```

### Create a New Branch (for features)
```bash
git checkout -b feature/feature-name
git push -u origin feature/feature-name
```

### View Remote Info
```bash
git remote -v
git branch -vv
```

## Troubleshooting

### "Permission denied (publickey)"
- SSH key not set up correctly
- Solution: Use HTTPS instead or reconfigure SSH key

### "fatal: 'origin' does not appear to be a 'git' repository"
- Remote not added correctly
- Solution: Run `git remote add origin ...` from Step 2

### "fatal: The current branch main has no upstream branch"
- First push needs `-u` flag
- Solution: `git push -u origin main`

### "Updates were rejected because the tip of your current branch is behind"
- Remote has changes you don't have locally
- Solution: `git pull origin main` then `git push`

## Verify Your Repository

After pushing, confirm everything is correct:

1. Visit: `https://github.com/YOUR_USERNAME/emcli`
2. Check commit count matches local: `git log --oneline | wc -l`
3. Verify files are present in file browser
4. Check that README.md renders correctly

## Next Steps

Once pushed to GitHub, you can:

1. **Enable GitHub Pages** for documentation (if desired)
2. **Add Topics**: Go to Settings → Add topics like `cli`, `embedded`, `c`
3. **Create Releases**: Tag important versions with `git tag`
4. **Enable Discussions** for community engagement
5. **Set up GitHub Actions** for CI/CD (optional)

## Repository URLs

After pushing:
- **HTTPS Clone**: `https://github.com/YOUR_USERNAME/emcli.git`
- **SSH Clone**: `git@github.com:YOUR_USERNAME/emcli.git`
- **Web URL**: `https://github.com/YOUR_USERNAME/emcli`

---

**Congratulations!** Your project is now on GitHub. Share the repository URL with others or start collaborating!
