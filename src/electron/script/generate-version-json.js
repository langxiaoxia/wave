const fs = require('fs');
const path = require('path');
const semver = require('semver');

const outputPath = process.argv[2];

const currentVersion = fs.readFileSync(path.resolve(__dirname, '../ELECTRON_VERSION'), 'utf8').trim();
const tag = fs.readFileSync(path.resolve(__dirname, '../ELECTRON_TAG'), 'utf8').trim(); //+by xxlang@2021-12-09

const parsed = semver.parse(currentVersion);

let prerelease = '';
if (parsed.prerelease && parsed.prerelease.length > 0) {
  prerelease = parsed.prerelease.join('.');
}

const {
  major,
  minor,
  patch
} = parsed;

fs.writeFileSync(outputPath, JSON.stringify({
  tag, //+by xxlang@2021-12-09
  major,
  minor,
  patch,
  prerelease,
  has_prerelease: prerelease === '' ? 0 : 1
}, null, 2));
