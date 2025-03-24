package emu;

import android.content.Context;
import android.content.res.AssetManager;
import android.os.Environment;

import system.Preferences;
import util.LogManager;
import util.Logger;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintStream;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collection;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/**
 * Created by roland on 31.08.2016.
 */

public class ImageManager {

	private static final Logger logger = LogManager.getLogger(ImageManager.class.getName());
	private static ImageManager globalInstance;

	private static final String [] DEFAULT_ARCHIVES = new String[] {
		"droid2600.zip", "vcs2600.zip", "atari2600.zip"
	};

	private static final String SNAPSHOT_DIR = "droid2600";
	private static final String SNAPSHOT_PREFIX = "vcs";

	private final List<Object> diskImageList = new ArrayList<Object>();
	private boolean dirty;
	private Image currentDiskImage;
	private String snapshotDir;
	private Context context;

	public static ImageManager instance() {
		return globalInstance;
	}

	public ImageManager() {
		globalInstance = this;
		dirty = true;
		initialize();
	}

	private void initialize() {
	}

	public void bindContext(Context context) {
		this.context = context;
		installDefaults();
		detectSnapshotDir();
		getStorageDirs();
	}

	/* Checks if external storage is available for read and write */
	private boolean isExternalStorageWritable() {
		String state = Environment.getExternalStorageState();
        return Environment.MEDIA_MOUNTED.equals(state);
    }

	/* Checks if external storage is available to at least read */
	private boolean isExternalStorageReadable() {
		String state = Environment.getExternalStorageState();
		if (Environment.MEDIA_MOUNTED.equals(state) ||
				Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
			return true;
		}
		return false;
	}

	private void installDefaults() {

		Context context = this.context;

        File dataDir = this.context.getDataDir();
		if (null == dataDir) {
			return;
		}

		File gamesDir = new File(dataDir, "games");
		if (!gamesDir.exists()) {
			if (!gamesDir.mkdirs()) {
				return;
			}
		}

		if (!gamesDir.isDirectory()) {
			return;
		}

		// now we have a ...data/games directory

		// copy demo archive
		copyAsset("droid2600.zip", gamesDir, false);
    }

	private void copyAsset(String assetName, File outputDir, boolean overwrite) {
		final File outputFile = new File(outputDir, assetName);

		if (!overwrite && outputFile.exists()) {
			return; // do not overwrite!!
		}

		final AssetManager am = context.getAssets();

		try (final FileOutputStream os = new FileOutputStream(outputFile)) {
			final InputStream is = am.open(assetName, AssetManager.ACCESS_BUFFER);

			final byte[] buffer = new byte[4096]; // = byte[4096];

			int sz;
			while ((sz = is.read(buffer, 0, buffer.length)) > 0) {
				os.write(buffer, 0, sz);
			}

			is.close();
			os.close();

		} catch (FileNotFoundException e) {
			// ignore, just do nothing
		} catch (IOException e) {
			//
		}
	}

	private String[] getStorageDirs() {
		PathList paths = new PathList();
		paths.add(this.context.getDataDir());
		paths.add(this.context.getDataDir(), "games", true);
		paths.add(this.context.getFilesDir());
		paths.add(this.context.getFilesDir(), "games", false);
		paths.add(this.context.getExternalFilesDir(null));
		paths.add(this.context.getExternalFilesDir("games"));
		paths.add(this.context.getObbDir());
		paths.add(this.context.getObbDir(), "games", true);
		paths.add(Environment.getDataDirectory());
		paths.add(Environment.getDataDirectory());
		paths.add(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS));
		paths.add(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS));
		paths.addString(getSnapshotDir());
		return paths.toArray();
	}

	private void scanDisks() {
		diskImageList.clear();
		String[] storageDirs = getStorageDirs();
		for (String folderName : storageDirs) {
			File folder = new File(folderName);
			if (folder.isDirectory()) {
				scanFolder(folder);
			}
		}
		dirty = false;
	}

	private void scanZipFile(File zip) {

		ZipFile zipFile = null;

		try {
			zipFile = new ZipFile(zip);
		} catch (IOException e) {
			logger.warning("could not scan zip file");
			return;
		}

		String zipPath = zip.getAbsolutePath();

		Enumeration zipEntries = zipFile.entries();
		while (zipEntries.hasMoreElements()) {
			ZipEntry entry = ((ZipEntry) zipEntries.nextElement());
			if (entry.isDirectory()) continue;

			String name = entry.getName();
			String ext = getFileExtension(name);

			if (isValidExtension(ext)) {
				logger.info("found disk in zip file: " + entry.getName());
				diskImageList.add(new Image(zipPath, name));
			}
		}
	}

	private void scanFolder(File folder) {
		if (null == folder || !folder.isDirectory()) {
			return;
		}

		logger.info(">>> scanning folder " + folder);

		Preferences prefs = Preferences.instance();
		final boolean scanZipFiles = null != prefs && prefs.isZipScanEnabled();

		File[] files = folder.listFiles(new FilenameFilter() {
			@Override
			public boolean accept(File dir, String name) {
				for (String archiveName : DEFAULT_ARCHIVES) {
					if (name.equalsIgnoreCase(archiveName)) {
						return true;
					}
				}

				String ext = getFileExtension(name);
				if (scanZipFiles && ext.equalsIgnoreCase("zip")) {
					return true;
				}

				return isValidExtension(ext);
			}});

		if (null == files || files.length == 0) {
			//logger.info("EMPTY DIR: " + folder.getAbsolutePath());
			return;
		}

		for (File file : files) {
			String url = file.getAbsolutePath();
			if (url.toLowerCase().endsWith(".zip")) {
				logger.info("scanning zip file: " + url);
				scanZipFile(file);
			} else {
				logger.info("found disk: " + url);
				diskImageList.add(new Image(url));
			}
		}
	}

	private String getFileExtension(String name) {
		if (null == name) return null;
		if (name.isEmpty()) return "";
		int pos = name.lastIndexOf('.');
		if (pos <= 0) return "";

		return name.substring(pos+1).toLowerCase();
	}

	public Image getCurrent() {
		return currentDiskImage;
	}

	public void setCurrent(Image image) {
		currentDiskImage = image;
	}

	public Collection<Object> getList(ImageFilter diskFilter) {
		if (dirty) {
			scanDisks();
		}

		if (null == diskFilter) {
			return diskImageList;
		}

		List<Object> filteredList = new ArrayList<Object>();
		for (Object o : diskImageList) {
			Image img = (Image) o;

			if (diskFilter.match(img)) {
				filteredList.add(img);
			}
		}

		return filteredList;
	}

	public void invalidateList() {
		dirty = true;
	}

	public static boolean isValidExtension(String ext) {

		if (ext.equalsIgnoreCase("bin") || ext.equalsIgnoreCase("a26") || ext.equalsIgnoreCase("snap")) {
			return true;
		}

		Preferences prefs = Preferences.instance();
		if (null == prefs) {
			return false;
		}

		return false;

	}

	private String detectSnapshotDir() {
		if (null == context) return null;

		File snapshotDir = null;
		if (isExternalStorageReadable() && isExternalStorageWritable()) {
			snapshotDir = context.getExternalFilesDir("Snapshots");
		} else {
			File baseDir = Environment.getDataDirectory();
			if (null == baseDir) {
				logger.warning("could not find location to store snapshots");
				return null;
			}
			logger.info("snapshot: base dir = " + baseDir.getAbsolutePath());
			snapshotDir = new File(baseDir, SNAPSHOT_DIR);
		}

		if (null == snapshotDir || snapshotDir.isFile()) {
			return null;
		}

		if (snapshotDir.exists() && !snapshotDir.isDirectory()) {
			return null;
		}

		if (!snapshotDir.isDirectory() && !snapshotDir.mkdirs()) {
			return null;
		}

		logger.info("snapshot dir = " + snapshotDir.getAbsolutePath());

		return snapshotDir.getAbsolutePath();
	}

	public String getSnapshotDir() {
		if (null == snapshotDir) {
			snapshotDir = detectSnapshotDir();
		}

		return snapshotDir;
	}

	public String getSnapshotName() {

		String name = SNAPSHOT_PREFIX;

		Image currentImage = getCurrent();
		if (null != currentImage) {

			String imageName = currentImage.getName();
			if (null != imageName && !imageName.isEmpty()) {

				if (currentImage.getType() == Image.TYPE_SNAPSHOT && imageName.startsWith(SNAPSHOT_PREFIX + "-")) {
					// do not use default snapshot name again.
				} else {

					int pos = imageName.lastIndexOf('.');
					if (pos >= 0) {
						imageName = imageName.substring(0, pos);
					}

					StringBuilder s = new StringBuilder();

					boolean newWord = true;
					for (char c : imageName.toCharArray()) {

						if (Character.isWhitespace(c) || c == '_') {
							newWord = true;
						} else {
							s.append(newWord ? Character.toUpperCase(c) : Character.toLowerCase(c));
							newWord = false;
						}
					}

					name = s.toString();
				}
			}
		}

		Calendar c = Calendar.getInstance(Locale.getDefault());
		SimpleDateFormat dateformat = new SimpleDateFormat("yyyyMMdd-HHmm");
		String datetime = dateformat.format(c.getTime());
		String filename = name + "-" + datetime + ".snap";

		return filename;
	}

	public boolean storeSnapshot(byte[] snapshotBuffer, int snapshotBufferUsage) {

		if (null == snapshotBuffer || snapshotBufferUsage < 1) return false;

		String snapshotDir = getSnapshotDir();
		if (null == snapshotDir) return false;

		String filename = getSnapshotName();

		File snapshot = new File(snapshotDir, filename);

		try {

			OutputStream os = new FileOutputStream(snapshot);

			os.write(snapshotBuffer, 0, snapshotBufferUsage);

			invalidateList();

			logger.info("stored snapshot: " + snapshot.getAbsolutePath());

			return true;

		} catch (FileNotFoundException e) {
			;
		} catch (IOException e) {
			;
		}

		return false;

	}

	/*
	public void attachImage(AssetManager assets, Image image) {

        try {
            InputStream is = assets.open(image.getUrl());

            byte[] buffer = new byte[DISK_IMAGE_SIZE];

            int bytesRead = is.read(buffer);
            if (bytesRead > 0) {
                attachImage(image.getType(), buffer, buffer.length, image.getUrl());
            }
        } catch (IOException e) {
            Logger.error("failed to read disk image from assets");
        }
	}*/
}


class PathList {

	private static final Logger logger = LogManager.getLogger(PathList.class.getName());
	private final Set<String> items = new HashSet<>();

	public PathList() {
	}

	public void add(File file) {
		if (null == file) {
			return;
		}
		this.add(file, null, false);
	}

	public void add(File file, String appendix, boolean create) {
		if (null == file) {
			return;
		}

		String path = file.getAbsolutePath();
		if (null != appendix && !appendix.isEmpty()) {
			path += "/" + appendix;
		}

		if (create) {
			File create_dir = new File(path);
			if(!create_dir.exists()){
				if (!create_dir.mkdirs()) {
					return;
				}
			}
		}

		this.addString(path);
	}

	public void addString(String file) {
		if (null == file || file.isEmpty()) {
			return;
		}
		//logger.info("added path to storage list: " + file);
		this.items.add(file);
	}

	public String[] toArray() {
		return this.items.toArray(new String[0]);
	}
}
