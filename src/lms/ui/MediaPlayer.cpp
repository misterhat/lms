/*
 * Copyright (C) 2018 Emeric Poupon
 *
 * This file is part of LMS.
 *
 * LMS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LMS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LMS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MediaPlayer.hpp"

#include "utils/Logger.hpp"

#include "database/Artist.hpp"
#include "database/Release.hpp"
#include "database/Track.hpp"
#include "database/User.hpp"

#include "resource/ImageResource.hpp"
#include "resource/AudioTranscodeResource.hpp"
#include "resource/AudioFileResource.hpp"

#include "utils/String.hpp"

#include "LmsApplication.hpp"

namespace UserInterface {


MediaPlayer::MediaPlayer()
: Wt::WTemplate {Wt::WString::tr("Lms.MediaPlayer.template")},
  playbackEnded {this, "playbackEnded"},
  playPrevious {this, "playPrevious"},
  playNext {this, "playNext"},
  _settingsLoaded {this, "settingsLoaded"}
{
	_title = bindNew<Wt::WText>("title");
	_artist = bindNew<Wt::WAnchor>("artist");
	_release = bindNew<Wt::WAnchor>("release");

	_settingsLoaded.connect([this](int mode, int format, int bitrate)
	{
		LMS_LOG(UI, DEBUG) << "Settings loaded! mode = " << mode << ", format = " << format << ", bitrate = " << bitrate;

		Settings settings;

		switch (static_cast<TranscodeMode>(mode))
		{
			case TranscodeMode::Always:
			case TranscodeMode::Never:
			case TranscodeMode::IfFormatNotSupported:
				settings.mode = static_cast<TranscodeMode>(mode);
				break;
		}

		switch (static_cast<Format>(format))
		{
			case Format::MP3:
			case Format::OGG_OPUS:
			case Format::MATROSKA_OPUS:
			case Format::OGG_VORBIS:
			case Format::WEBM_VORBIS:
				settings.format = static_cast<Format>(format);
				break;
		}

		if (Database::User::audioTranscodeAllowedBitrates.find(bitrate) != std::cend(Database::User::audioTranscodeAllowedBitrates))
			settings.bitrate = bitrate;

		_settings = settings;

		LmsApp->getEvents().mediaPlayerSettingsAvailable.emit();
	});

	{
		std::ostringstream oss;
		oss << "LMS.mediaplayer.init("
			<< jsRef()
			<< ", " << static_cast<int>(defaultTranscodeMode)
			<< ", " << static_cast<int>(defaultTranscodeFormat)
			<< ", " << static_cast<int>(defaultTranscodeBitrate)
			<< ")";

		LMS_LOG(UI, DEBUG) << "Running js = '" << oss.str() << "'";
		doJavaScript(oss.str());
	}

	LmsApp->getEvents().trackLoaded.connect(this, &MediaPlayer::loadTrack);
	LmsApp->getEvents().trackUnloaded.connect(this, &MediaPlayer::stop);
}

void
MediaPlayer::loadTrack(Database::IdType trackId, bool play)
{
	LMS_LOG(UI, DEBUG) << "Playing track ID = " << trackId;

	auto transaction {LmsApp->getDbSession().createSharedTransaction()};

	const auto track {Database::Track::getById(LmsApp->getDbSession(), trackId)};
	const std::string imgResourceMimeType {LmsApp->getImageResource()->getMimeType()};

	const std::string transcodeResource {LmsApp->getAudioTranscodeResource()->getUrl(trackId)};
	const std::string nativeResource {LmsApp->getAudioFileResource()->getUrl(trackId)};

	const auto artists {track->getArtists()};

	std::ostringstream oss;
	oss
		<< "var params = {"
		<< " native_resource: \"" << nativeResource << "\","
		<< " transcode_resource: \"" << transcodeResource << "\","
		<< " duration: " << std::chrono::duration_cast<std::chrono::seconds>(track->getDuration()).count() << ","
		<< " title: \"" << StringUtils::jsEscape(track->getName()) << "\","
		<< " artist: \"" << (!artists.empty() ? StringUtils::jsEscape(artists.front()->getName()) : "") << "\","
		<< " release: \"" << (track->getRelease() ? StringUtils::jsEscape(track->getRelease()->getName()) : "") << "\","
		<< " artwork: ["
		<< "   { src: \"" << LmsApp->getImageResource()->getTrackUrl(trackId, 96) << "\",  sizes: \"96x96\",	type: \"" << imgResourceMimeType << "\" },"
		<< "   { src: \"" << LmsApp->getImageResource()->getTrackUrl(trackId, 256) << "\", sizes: \"256x256\",	type: \"" << imgResourceMimeType << "\" },"
		<< "   { src: \"" << LmsApp->getImageResource()->getTrackUrl(trackId, 512) << "\", sizes: \"512x512\",	type: \"" << imgResourceMimeType << "\" },"
		<< " ]"
		<< "};";
	oss << "LMS.mediaplayer.loadTrack(params, " << (play ? "true" : "false") << ")"; // true to autoplay

	LMS_LOG(UI, DEBUG) << "Running js = '" << oss.str() << "'";

	_title->setTextFormat(Wt::TextFormat::Plain);
	_title->setText(Wt::WString::fromUTF8(track->getName()));

	if (!artists.empty())
	{
		_artist->setTextFormat(Wt::TextFormat::Plain);
		_artist->setText(Wt::WString::fromUTF8(artists.front()->getName()));
		_artist->setLink(LmsApp->createArtistLink(artists.front()));
	}
	else
	{
		_artist->setText("");
		_artist->setLink({});
	}

	if (track->getRelease())
	{
		_release->setTextFormat(Wt::TextFormat::Plain);
		_release->setText(Wt::WString::fromUTF8(track->getRelease()->getName()));
		_release->setLink(LmsApp->createReleaseLink(track->getRelease()));
	}
	else
	{
		_release->setText("");
		_release->setLink({});
	}

	wApp->doJavaScript(oss.str());
}

void
MediaPlayer::stop()
{
	wApp->doJavaScript("LMS.mediaplayer.stop()");
}

void
MediaPlayer::setSettings(const Settings& settings)
{
	_settings = settings;

	{
		std::ostringstream oss;
		oss << "LMS.mediaplayer.setSettings("
			<< static_cast<int>(_settings->mode)
			<< ", " << static_cast<int>(_settings->format)
			<< ", " << static_cast<int>(_settings->bitrate)
			<< ")";

		LMS_LOG(UI, DEBUG) << "Running js = '" << oss.str() << "'";
		doJavaScript(oss.str());
	}
}

} // namespace UserInterface

