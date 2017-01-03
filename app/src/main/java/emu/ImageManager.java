package emu;

import android.content.Context;
import android.os.Environment;

import system.Preferences;
import util.LogManager;
import util.Logger;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collection;
import java.util.Enumeration;
import java.util.List;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/**
 * Created by roland on 31.08.2016.
 */

public class ImageManager {

	private static final Logger logger = LogManager.getLogger(ImageManager.class.getName());
	private static ImageManager globalInstance;

	private static final String [] SCAN_SUBDIRS = new String[] {
		//"droid64", "c64", "Download", "Documents", "Download/droid64", "Documents/droid64", "Download/c64", "Documents/c64"
			"droid2600", "vcs", "Download", "Documents", "Download/droid2600", "Documents/droid2600", "Download/vcs", "Documents/vcs"
	};

	private static final String [] DEFAULT_ARCHIVES = new String[] {
			// "droid64.zip", "c64.zip"
			"droid2600.zip", "vcs.zip"
	};

	private static final String SNAPSHOT_DIR = "droid64";
	private static final String SNAPSHOT_PREFIX = "vcs"; // "c64";
	private static final String IMAGE_EXTENSION = "bin"; // "d64";

	private List<Object> diskImageList = new ArrayList<Object>();
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

		detectSnapshotDir();
	}

	/* Checks if external storage is available for read and write */
	private boolean isExternalStorageWritable() {
		String state = Environment.getExternalStorageState();
		if (Environment.MEDIA_MOUNTED.equals(state)) {
			return true;
		}
		return false;
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

	private String detectSnapshotDir() {

		if (null == context) return null;

		File snapshotDir = null;

		if (isExternalStorageReadable() && isExternalStorageWritable()) {

			//baseDir = Environment.getExternalStorageDirectory();

			snapshotDir = context.getExternalFilesDir("Snapshots");

		} else {

			File baseDir = Environment.getDataDirectory();

			if (null == baseDir) {
				logger.warning("could not find location to store snapshots");
			}

			logger.info("snapshot: base dir = " + baseDir.getAbsolutePath());
			snapshotDir = new File(baseDir, SNAPSHOT_DIR);
		}

		if (snapshotDir.isFile()) {
			return null;
		}

		if (snapshotDir.exists() && !snapshotDir.isDirectory()) {
			return null;
		}

		if (!snapshotDir.isDirectory() && false == snapshotDir.mkdirs()) {
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

	private String[] getStorageDirs() {

		StringBuilder paths = new StringBuilder();

		String path;

		path = System.getenv("EXTERNAL_STORAGE");
		if (null != path) paths.append(path);
		path = System.getenv("SECONDARY_STORAGE");
		if (null != path) { paths.append(':'); paths.append(path); }
		path = Environment.getDataDirectory().getAbsolutePath();
		if (null != path) { paths.append(':'); paths.append(path); }
		path = Environment.getDownloadCacheDirectory().getAbsolutePath();
		if (null != path) { paths.append(':'); paths.append(path); }
		path = getSnapshotDir();
		if (null != path) { paths.append(':'); paths.append(path); }

		String[] list = paths.toString().split(":");

		List<String> storageDirs = new ArrayList<String>();
		for (String item : list) {

			String dirItem = item.trim();
			if (!dirItem.isEmpty() && !storageDirs.contains(dirItem)) {
				storageDirs.add(dirItem);
			}
		}

		return storageDirs.toArray(new String[0]);
	}

	private void scanDisks() {

		diskImageList.clear();

		String[] storageDirs = getStorageDirs();

		String [] subDirs = SCAN_SUBDIRS;

		for (String folderName : storageDirs) {

			scan(folderName);

			for (String subDir : subDirs) {
				String subPath = folderName + (folderName.endsWith("/") ? "" : "/") + subDir;
				scan(subPath);
			}

		}

		dirty = false;

	}

	private void scan(String folderName) {
		File folder = new File(folderName);

		if (null == folder) {
			return;
		}

		if (folder.isDirectory()) {
			//logger.info("scanning folder " + folderName);
			scanFolder(folder);
		}
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
				//logger.info("found disk in zip file: " + entry.getName());
				diskImageList.add(new Image(zipPath, name));
			}
		}
	}

	private void scanFolder(File folder) {

		if (null == folder) {
			return;
		}

		if (!folder.isDirectory()) {
			return;
		}


		Preferences prefs = Preferences.instance();
		final boolean scanZipFiles = (null != prefs) ? prefs.isZipScanEnabled() : false;

		File[] files = folder.listFiles(new FilenameFilter() {
			@Override
			public boolean accept(File dir, String name) {

				for (String archiveName : DEFAULT_ARCHIVES) {
					if (name.equalsIgnoreCase(archiveName)) {
						return true;
					}
				}

				//logger.info("FILE: " + name);

				String ext = getFileExtension(name);

				if (scanZipFiles && ext.equalsIgnoreCase("zip")) {
					return true;
				}

				return isValidExtension(ext);

			}});

		if (null == files) {
			//logger.info("EMPTY DIR: " + folder.getAbsolutePath());
			return;
		}

		for (File file : files) {

			String url = file.getAbsolutePath();

			if (url.toLowerCase().endsWith(".zip")) {
				//logger.info("scanning zip file: " + url);
				scanZipFile(file);
			} else {
				//logger.info("found disk: " + url);
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

		if (ext.equalsIgnoreCase(IMAGE_EXTENSION) || ext.equalsIgnoreCase("snap")) {
			return true;
		}

		Preferences prefs = Preferences.instance();
		if (null == prefs) {
			return false;
		}

		return false;

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
